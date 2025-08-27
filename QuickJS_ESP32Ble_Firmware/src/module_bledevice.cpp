#include <Arduino.h>
#include "main_config.h"

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"

#ifdef _BLEDEVICE_ENABLE_

#include <NimBLEDevice.h>
#include "module_bledevice.h"

static NimBLEAdvertising *g_pAdvertising = NULL;
static NimBLEScan *g_pScan = NULL;
static NimBLEClient *g_pClient = NULL;

#define BLEBEACON_RUNNING_NONE  0
#define BLEBEACON_RUNNING_ADVERTISE 1
#define BLEBEACON_RUNNING_SCAN      2
static int isRunning = BLEBEACON_RUNNING_NONE;

typedef struct{
  char *p_uuid;
  uint8_t *p_data;
  uint32_t length;
  bool isNotify;
} BLE_NOTIFY_INFO;

static NimBLEScanCallbacks *g_pcallback = NULL;
static std::vector<BLE_NOTIFY_INFO> g_notify_list;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static JSValue g_callback_char_func = JS_UNDEFINED;
static bool g_scanCompleted = false;

static void taskServer(void *);

#define BLE_CAP_NONE              0x0000
#define BLE_CAP_READ              0x0001
#define BLE_CAP_WRITE             0x0002
#define BLE_CAP_WRITE_NORESPONSE  0x0004
#define BLE_CAP_BROADCAST         0x0008
#define BLE_CAP_INDICATE          0x0010
#define BLE_CAP_WRITE_SIGNED      0x0020

static uint32_t toCapability(NimBLERemoteCharacteristic* pChar)
{
  uint32_t cap = 0;
  if( pChar->canRead() ) cap |= BLE_CAP_READ;
  if( pChar->canWrite() ) cap |= BLE_CAP_WRITE;
  if( pChar->canWriteNoResponse() ) cap |= BLE_CAP_WRITE_NORESPONSE;
  if( pChar->canBroadcast() ) cap |= BLE_CAP_BROADCAST;
  if( pChar->canIndicate() ) cap |= BLE_CAP_INDICATE;
  if( pChar->canWriteSigned() ) cap |= BLE_CAP_WRITE_SIGNED;

  return cap;
}

static void disconnect()
{
  if( g_pClient != NULL ){
    if (g_pClient->isConnected()) {
        g_pClient->disconnect();
    }
    NimBLEDevice::deleteClient(g_pClient);
    g_pClient = NULL;
  }

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
    g_pScan->clearResults();
  }

  if( g_callback_char_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_char_func);
    g_callback_char_func = JS_UNDEFINED;

    while(g_notify_list.size() > 0){
      BLE_NOTIFY_INFO info = (BLE_NOTIFY_INFO)g_notify_list.front();
      free(info.p_uuid);
      free(info.p_data);
      g_notify_list.erase(g_notify_list.begin());
    }
  }
}

void myNotifyHandler(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t length, bool isNotify) {
//  Serial.print("Notify received: ");
  
  BLE_NOTIFY_INFO info;
    
  info.p_uuid = strdup(pChar->getUUID().toString().c_str());
  if( info.p_uuid == NULL )
    return;
  info.p_data = (uint8_t*)malloc(length);
  if( info.p_data == NULL ){
    free(info.p_uuid);
    return;
  }
  memmove(info.p_data, data, length);
  info.length = length;
  info.isNotify = isNotify;

  g_notify_list.push_back(info);
}

class MyScanCallbacks: public NimBLEScanCallbacks
{
  // void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
  // }
  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override{
    g_scanCompleted = true;
  }
};

static JSValue bledevice_disconnect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_UNDEFINED;

  if (g_pClient->isConnected()) {
    g_pClient->disconnect();
  }
  NimBLEDevice::deleteClient(g_pClient);
  g_pClient = NULL;

  return JS_UNDEFINED;
}

static JSValue bledevice_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *p_address = JS_ToCString(ctx, argv[0]);
  std::string s_address(p_address);
  int32_t type;
  JS_ToInt32(ctx, &type, argv[1]);
  NimBLEAddress address(s_address, type);
  JS_FreeCString(ctx, p_address);

  NimBLEClient* pClient = NimBLEDevice::createClient();
  if( g_pClient != NULL ){
    if (g_pClient->isConnected()) {
        g_pClient->disconnect();
    }
    NimBLEDevice::deleteClient(g_pClient);
    g_pClient = NULL;
  }

  NimBLEDevice::setSecurityAuth(true, false, false);  // bondingのみ
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);  // Just Works

  bool result = pClient->connect(address);
  if( result ){
    g_pClient = pClient;
  }else{
    NimBLEDevice::deleteClient(pClient);
  }

  return JS_NewBool(ctx, result);
}

static JSValue bledevice_listPrimaryService(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_EXCEPTION;

    std::vector<NimBLERemoteService*> services = g_pClient->getServices();
    JSValue array = JS_NewArray(ctx);
    for( int i = 0 ; i < services.size() ; i++ ){
      NimBLERemoteService* pService = services[i];
      JS_SetPropertyUint32(ctx, array, i, JS_NewString(ctx, pService->getUUID().toString().c_str()));
    }

    return array;
}

static JSValue bledevice_listCharacteristic(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_EXCEPTION;

  const char* serviceUuid = JS_ToCString(ctx, argv[0]);
  if( serviceUuid == NULL )
    return JS_EXCEPTION;

  NimBLERemoteService* pService = g_pClient->getService(serviceUuid);
  if( pService == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    return JS_EXCEPTION;
  }
  JS_FreeCString(ctx, serviceUuid);

  std::vector<NimBLERemoteCharacteristic*> chars = pService->getCharacteristics();

  JSValue array = JS_NewArray(ctx);
  for( int i = 0 ; i < chars.size() ; i++ ){
    NimBLERemoteCharacteristic* pChar = chars[i];
    if( pChar == NULL )
      continue;

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "uuid", JS_NewString(ctx, pChar->getUUID().toString().c_str()));
    JS_SetPropertyStr(ctx, obj, "cap", JS_NewUint32(ctx, toCapability(pChar)));
    JS_SetPropertyUint32(ctx, array, i, obj);
  }

  return array;
}

static JSValue bledevice_write(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_EXCEPTION;

  const char* serviceUuid = JS_ToCString(ctx, argv[0]);
  if( serviceUuid == NULL )
    return JS_EXCEPTION;
  const char* characteristicUuid = JS_ToCString(ctx, argv[1]);
  if( characteristicUuid == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    return JS_EXCEPTION;
  }

  uint8_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[2], (void**)&p_buffer, &unit_size, &unit_num);
  if( vbuffer == JS_NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }
  if( unit_size != 1 ){
    JS_FreeValue(ctx, vbuffer);
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }

  bool response = JS_ToBool(ctx, argv[3]);

  NimBLERemoteService* pService = g_pClient->getService(serviceUuid);
  if( pService == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }
  NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(characteristicUuid);
  if( pChar == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  JS_FreeCString(ctx, serviceUuid);
  JS_FreeCString(ctx, characteristicUuid);

  if( (!response && !pChar->canWriteNoResponse()) || (response && !pChar->canWrite()) )
    return JS_EXCEPTION;

  bool result = pChar->writeValue(p_buffer, unit_num, response);
  JS_FreeValue(ctx, vbuffer);

  return JS_NewBool(ctx, result);
}

static JSValue bledevice_read(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_EXCEPTION;

  const char* serviceUuid = JS_ToCString(ctx, argv[0]);
  if( serviceUuid == NULL )
    return JS_EXCEPTION;
  const char* characteristicUuid = JS_ToCString(ctx, argv[1]);
  if( characteristicUuid == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    return JS_EXCEPTION;
  }

  NimBLERemoteService* pService = g_pClient->getService(serviceUuid);
  if( pService == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }
  NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(characteristicUuid);
  if( pChar == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }

  JS_FreeCString(ctx, serviceUuid);
  JS_FreeCString(ctx, characteristicUuid);

  if( !pChar->canRead() )
    return JS_EXCEPTION;

  std::string raw = pChar->readValue();
  const uint8_t* data = reinterpret_cast<const uint8_t*>(raw.data());
  size_t len = raw.length();

  JSValue value = JS_EXCEPTION;
  value = JS_NewArrayBufferCopy(ctx, data, len);

  return value;
}

static JSValue bledevice_subscribe(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( g_pClient == NULL || !g_pClient->isConnected() )
    return JS_EXCEPTION;

  const char* serviceUuid = JS_ToCString(ctx, argv[0]);
  if( serviceUuid == NULL )
    return JS_EXCEPTION;
  const char* characteristicUuid = JS_ToCString(ctx, argv[1]);
  if( characteristicUuid == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    return JS_EXCEPTION;
  }

  bool response = JS_ToBool(ctx, argv[2]);

  NimBLERemoteService* pService = g_pClient->getService(serviceUuid);
  if( pService == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }
  NimBLERemoteCharacteristic* pChar = pService->getCharacteristic(characteristicUuid);
  if( pChar == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }

  JS_FreeCString(ctx, serviceUuid);
  JS_FreeCString(ctx, characteristicUuid);

  bool result = pChar->subscribe(magic == 0 ? true : false, myNotifyHandler, response);
  return JS_NewBool(ctx, result);;
}

static JSValue bledevice_startScan(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLEBEACON_RUNNING_NONE)
    return JS_EXCEPTION;    
  if( argc < 3 )
    return JS_EXCEPTION;    

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  JSValue value;
  value = JS_GetPropertyStr(ctx, argv[1], "activeScan");
  if( value != JS_UNDEFINED ){
    int active;
    active = JS_ToBool(ctx, value);
    JS_FreeValue(ctx, value);
    g_pScan->setActiveScan(active);
  }

  value = JS_GetPropertyStr(ctx, argv[1], "interval");
  if( value != JS_UNDEFINED ){
    uint32_t interval;
    JS_ToUint32(ctx, &interval, value);
    JS_FreeValue(ctx, value);
    g_pScan->setInterval(interval);
  }

  value = JS_GetPropertyStr(ctx, argv[1], "window");
  if( value != JS_UNDEFINED ){
    uint32_t window;
    JS_ToUint32(ctx, &window, value);
    JS_FreeValue(ctx, value);
    g_pScan->setWindow(window);
  }

  g_ctx = ctx;
  g_callback_func = JS_DupValue(g_ctx, argv[2]);

  g_scanCompleted = false;
  g_pScan->clearResults();

  g_pScan->start(duration);
  isRunning = BLEBEACON_RUNNING_SCAN;

  return JS_UNDEFINED;
}

static JSValue bledevice_startAdvertise2(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLEBEACON_RUNNING_NONE )
    return JS_EXCEPTION;    

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  JSValue value;
  if( argc >= 2 ){
    NimBLEAdvertisementData advertisementData = NimBLEAdvertisementData();

    value = JS_GetPropertyStr(ctx, argv[1], "flags");
    if( value != JS_UNDEFINED ){
      uint32_t flags;
      JS_ToUint32(ctx, &flags, value);
      JS_FreeValue(ctx, value);
      advertisementData.setFlags(flags);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      advertisementData.setCompleteServices(uuid);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      advertisementData.setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      advertisementData.setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "manufacturerData");
    if( value != JS_UNDEFINED ){
      uint8_t *p_buffer;
      uint8_t unit_size;
      uint32_t unit_num;
      JSValue vbuffer = getTypedArrayBuffer(ctx, value, (void**)&p_buffer, &unit_size, &unit_num);
      if( JS_IsNull(vbuffer) ){
        return JS_EXCEPTION;
      }
      if( unit_size != 1 ){
        JS_FreeValue(ctx, vbuffer);
        return JS_EXCEPTION;
      }

      JS_FreeValue(ctx, value);

      advertisementData.setManufacturerData((char*)p_buffer);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "rawData");
    if( value != JS_UNDEFINED ){
      JSValue jv = JS_GetPropertyStr(ctx, value, "length");
      uint32_t length;
      JS_ToUint32(ctx, &length, jv);
      JS_FreeValue(ctx, jv);

      uint8_t *p_buffer = (uint8_t*)malloc(length);
      if( p_buffer == NULL ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      for (uint32_t i = 0; i < length; i++){
        JSValue jv = JS_GetPropertyUint32(ctx, value, i);
        uint32_t item;
        JS_ToUint32(ctx, &item, jv);
        JS_FreeValue(ctx, jv);
        p_buffer[i] = item;
      }
      JS_FreeValue(ctx, value);

      advertisementData.addData(p_buffer, length);
      free(p_buffer);
    }

    g_pAdvertising->setAdvertisementData(advertisementData);

    // advertisementData.setFlags(0x06);
    // advertisementData.setCompleteServices(BLEUUID("fe6f"));
    // unsigned char data2[] = {11, 0x16, 0x6f, 0xfe, 0x02, 0x01, 0x21, 0x3a, 0x72, 0x4f, 0x7f, 0x20 };
    // advertisementData.addData((char*)data2, sizeof(data2));
    // g_pAdvertising->setAdvertisementData(advertisementData);
  }

  if( argc >= 3 ){
    NimBLEAdvertisementData scanResponseData = NimBLEAdvertisementData();

    value = JS_GetPropertyStr(ctx, argv[2], "flags");
    if( value != JS_UNDEFINED ){
      uint32_t flags;
      JS_ToUint32(ctx, &flags, value);
      JS_FreeValue(ctx, value);
      scanResponseData.setFlags(flags);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      scanResponseData.setCompleteServices(uuid);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      scanResponseData.setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      scanResponseData.setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "manufacturerData");
    if( value != JS_UNDEFINED ){
      uint8_t *p_buffer;
      uint8_t unit_size;
      uint32_t unit_num;
      JSValue vbuffer = getTypedArrayBuffer(ctx, value, (void**)&p_buffer, &unit_size, &unit_num);
      if( JS_IsNull(vbuffer) ){
        return JS_EXCEPTION;
      }
      if( unit_size != 1 ){
        JS_FreeValue(ctx, vbuffer);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      scanResponseData.setManufacturerData((char*)p_buffer);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "rawData");
    if( value != JS_UNDEFINED ){
      JSValue jv = JS_GetPropertyStr(ctx, value, "length");
      uint32_t length;
      JS_ToUint32(ctx, &length, jv);
      JS_FreeValue(ctx, jv);

      uint8_t *p_buffer = (uint8_t*)malloc(length);
      if( p_buffer == NULL ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      for (uint32_t i = 0; i < length; i++){
        JSValue jv = JS_GetPropertyUint32(ctx, value, i);
        uint32_t item;
        JS_ToUint32(ctx, &item, jv);
        JS_FreeValue(ctx, jv);
        p_buffer[i] = item;
      }
      JS_FreeValue(ctx, value);

      scanResponseData.addData(p_buffer, length);
      free(p_buffer);
    }

    g_pAdvertising->setScanResponseData(scanResponseData);
  }

  g_pAdvertising->start();
  isRunning = BLEBEACON_RUNNING_ADVERTISE;

  return JS_UNDEFINED;
}

static JSValue bledevice_startAdvertise(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLEBEACON_RUNNING_NONE)
    return JS_EXCEPTION;    

  g_pAdvertising->reset();
  g_pAdvertising->removeServices();

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  if( argc >= 2 ){
    JSValue value;

    value = JS_GetPropertyStr(ctx, argv[1], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      g_pAdvertising->addServiceUUID(uuid);
    }
  
    value = JS_GetPropertyStr(ctx, argv[1], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      g_pAdvertising->setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "manufacturerData");
    if( value != JS_UNDEFINED ){
      JSValue jv = JS_GetPropertyStr(ctx, value, "length");
      uint32_t length;
      JS_ToUint32(ctx, &length, jv);
      JS_FreeValue(ctx, jv);

      std::vector<uint8_t> buffer;
      for (uint32_t i = 0; i < length; i++){
        JSValue jv = JS_GetPropertyUint32(ctx, value, i);
        uint32_t item;
        JS_ToUint32(ctx, &item, jv);
        JS_FreeValue(ctx, jv);
        buffer.push_back(item);
      }
      JS_FreeValue(ctx, value);

      g_pAdvertising->setManufacturerData(buffer);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "maxInterval");
    if( value != JS_UNDEFINED ){
      uint32_t maxInterval;
      JS_ToUint32(ctx, &maxInterval, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setMaxInterval(maxInterval);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "minInterval");
    if( value != JS_UNDEFINED ){
      uint32_t minInterval;
      JS_ToUint32(ctx, &minInterval, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setMaxInterval(minInterval);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "minPreferred");
    if( value != JS_UNDEFINED ){
      uint32_t minPreferred;
      JS_ToUint32(ctx, &minPreferred, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setMaxInterval(minPreferred);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "maxPreffered");
    if( value != JS_UNDEFINED ){
      uint32_t maxPreffered;
      JS_ToUint32(ctx, &maxPreffered, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setMaxInterval(maxPreffered);
    }    

    value = JS_GetPropertyStr(ctx, argv[1], "addTxPower");
    if( value != JS_UNDEFINED ){
      int addTxPower = JS_ToBool(ctx, value);
      JS_FreeValue(ctx, value);
      if( addTxPower )
        g_pAdvertising->addTxPower();
    }
  }

  g_pAdvertising->start(duration);
  isRunning = BLEBEACON_RUNNING_ADVERTISE;

  return JS_UNDEFINED;
}

static JSValue bledevice_setPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t power;
  JS_ToInt32(ctx, &power, argv[0]);

  esp_power_level_t level;

  switch(power){
    case -12: level = ESP_PWR_LVL_N12; break;
    case -9: level = ESP_PWR_LVL_N9; break;
    case -6: level = ESP_PWR_LVL_N6; break;
    case -3: level = ESP_PWR_LVL_N3; break;
    case 0: level = ESP_PWR_LVL_N0; break;
    case 3: level = ESP_PWR_LVL_P3; break;
    case 6: level = ESP_PWR_LVL_P6; break;
    case 9: level = ESP_PWR_LVL_P9; break; // default
    default: return JS_EXCEPTION;
  }

  BLEDevice::setPower(level);

  return JS_UNDEFINED;
}

static JSValue bledevice_getPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int power = BLEDevice::getPower();

  return JS_NewInt32(ctx, power);
}

static JSValue bledevice_setMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t mtu;
  JS_ToUint32(ctx, &mtu, argv[0]);

  BLEDevice::setMTU(mtu);

  return JS_UNDEFINED;
}

static JSValue bledevice_getMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t mtu = BLEDevice::getMTU();

  return JS_NewUint32(ctx, mtu);
}

static void blebeacon_stopAll(void){
  if( isRunning == BLEBEACON_RUNNING_ADVERTISE ){
    g_pAdvertising->stop();
    isRunning = 0;
  }else if( isRunning == BLEBEACON_RUNNING_SCAN ){
    g_pScan->stop();
    isRunning = 0;
  }
}

static JSValue bledevice_stop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  blebeacon_stopAll();

  return JS_UNDEFINED;
}

static JSValue bledevice_getMacAddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const uint8_t *p_address = BLEDevice::getAddress().getVal();

  JSValue jsArray = JS_NewArray(ctx);
  for (int i = 0; i < 6; i++)
    JS_SetPropertyUint32(ctx, jsArray, i, JS_NewInt32(ctx, p_address[5 - i]));

  return jsArray;
}

static JSValue bledevice_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_callback_char_func != JS_UNDEFINED )
    JS_FreeValue(g_ctx, g_callback_char_func);

  g_ctx = ctx;
  g_callback_char_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static JSValue bledevice_setSecurity(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool bonding, mitm, sc;
  bonding = JS_ToBool(ctx, argv[0]);
  mitm = JS_ToBool(ctx, argv[1]);
  sc = JS_ToBool(ctx, argv[2]);
  uint32_t iocap;
  JS_ToUint32(ctx, &iocap, argv[3]);

  NimBLEDevice::setSecurityAuth(bonding, mitm, sc);
  NimBLEDevice::setSecurityIOCap(iocap);

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry bledevice_funcs[] = {
    JSCFunctionListEntry{
        "connect", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bledevice_connect}
        }},
    JSCFunctionListEntry{
        "disconnect", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_disconnect}
        }},
    JSCFunctionListEntry{
        "listPrimaryService", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_listPrimaryService}
        }},
    JSCFunctionListEntry{
        "listCharacteristic", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bledevice_listCharacteristic}
        }},
    JSCFunctionListEntry{
        "write", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, bledevice_write}
        }},
    JSCFunctionListEntry{
        "read", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bledevice_read}
        }},
    JSCFunctionListEntry{
        "subscribe", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic_magic, {generic_magic : bledevice_subscribe}}
        }},
    JSCFunctionListEntry{
        "unsubscribe", 1, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic_magic, {generic_magic : bledevice_subscribe}}
        }},

    JSCFunctionListEntry{
        "startScan", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, bledevice_startScan}
        }},
    JSCFunctionListEntry{
        "startAdvertise", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bledevice_startAdvertise}
        }},
    JSCFunctionListEntry{
        "startAdvertise2", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, bledevice_startAdvertise2}
        }},
    JSCFunctionListEntry{
        "stop", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_stop}
        }},
    JSCFunctionListEntry{
        "setPower", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bledevice_setPower}
        }},
    JSCFunctionListEntry{
        "getPower", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_getPower}
        }},
    JSCFunctionListEntry{
        "setMtu", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bledevice_setMtu}
        }},
    JSCFunctionListEntry{
        "getMtu", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_getMtu}
        }},
    JSCFunctionListEntry{
        "getMacAddress", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bledevice_getMacAddress}
        }},
    JSCFunctionListEntry{
        "setSecurity", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, bledevice_setSecurity}
        }},

    JSCFunctionListEntry{
        "ADV_NONCONN_IND", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_GAP_CONN_MODE_NON
        }},    
    JSCFunctionListEntry{
        "ADV_IND", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_GAP_CONN_MODE_UND
        }},    
    JSCFunctionListEntry{
        "ADV_DIRECT_IND", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_GAP_CONN_MODE_DIR
        }},    

    JSCFunctionListEntry{
        "CAP_NONE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_NONE
        }},    
    JSCFunctionListEntry{
        "CAP_READ", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_READ
        }},    
    JSCFunctionListEntry{
        "CAP_WRITE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_WRITE
        }},    
    JSCFunctionListEntry{
        "CAP_WRITE_NORESPONSE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_WRITE_NORESPONSE
        }},    
    JSCFunctionListEntry{
        "CAP_WRITE_SIGNED", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_WRITE_SIGNED
        }},    
    JSCFunctionListEntry{
        "CAP_BROADCAST", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_BROADCAST
        }},
    JSCFunctionListEntry{
        "CAP_INDICATE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_CAP_INDICATE
        }},
    JSCFunctionListEntry{
        "IOCAP_DISPLAY_ONLY", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_HS_IO_DISPLAY_ONLY
        }},    
    JSCFunctionListEntry{
        "IOCAP_DISPLAY_YESNO", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_HS_IO_DISPLAY_YESNO
        }},    
    JSCFunctionListEntry{
        "IOCAP_KEYBOARD_ONLY", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_HS_IO_KEYBOARD_ONLY
        }},    
    JSCFunctionListEntry{
        "IOCAP_NO_INPUT_OUTPUT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_HS_IO_NO_INPUT_OUTPUT
        }},    
    JSCFunctionListEntry{
        "IOCAP_KEYBOARD_DISPLAY", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BLE_HS_IO_KEYBOARD_DISPLAY
        }},    
};

JSModuleDef *addModule_blecentral(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "BleDevice", [](JSContext *ctx, JSModuleDef *m){
          return JS_SetModuleExportList(
                      ctx, m, bledevice_funcs,
                      sizeof(bledevice_funcs) / sizeof(JSCFunctionListEntry));
          });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, bledevice_funcs,
        sizeof(bledevice_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initializeModule_blecentral(void)
{
  taskServer(NULL);

  // BaseType_t ret = xTaskCreate(taskServer, "server", 4096, NULL, 5, NULL);
  // if( ret != pdPASS ){
  //   Serial.printf("xTaskCreate error=%d\n", ret);
  // }

  return 0;
}

void endModule_blecentral(void){
  blebeacon_stopAll();

  disconnect();

  // default
  BLEDevice::setPower(ESP_PWR_LVL_P9);
  BLEDevice::setMTU(255);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
}

void loopModule_blecentral(void){
  if( g_callback_char_func != JS_UNDEFINED ){
    while(g_notify_list.size() > 0){
        BLE_NOTIFY_INFO info = (BLE_NOTIFY_INFO)g_notify_list.front();
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "uuid", JS_NewString(g_ctx, info.p_uuid));
        JS_SetPropertyStr(g_ctx, obj, "data", JS_NewArrayBufferCopy(g_ctx, info.p_data, info.length));
        JS_SetPropertyStr(g_ctx, obj, "is_notify", JS_NewBool(g_ctx, info.isNotify));
        free(info.p_uuid);
        free(info.p_data);
        g_notify_list.erase(g_notify_list.begin());

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }
  }
  if( g_scanCompleted && g_callback_func != JS_UNDEFINED ){
    NimBLEScanResults results = g_pScan->getResults();
    JSValue array = JS_NewArray(g_ctx);
    for( int i = 0 ; i < results.getCount(); i++ ){
      const BLEAdvertisedDevice* p_device = results.getDevice(i);
      JSValue item = JS_NewObject(g_ctx);

      JS_SetPropertyStr(g_ctx, item, "address", JS_NewString(g_ctx, p_device->getAddress().toString().c_str()));
      const std::vector<uint8_t>& payload = p_device->getPayload();
      uint8_t* raw = (uint8_t*)malloc(payload.size());
      if( raw != NULL ){
        std::copy(payload.begin(), payload.end(), raw);
        JS_SetPropertyStr(g_ctx, item, "advertisement", JS_NewArrayBufferCopy(g_ctx, raw, payload.size()));
        free(raw);
      }
      JS_SetPropertyStr(g_ctx, item, "rssi", JS_NewInt32(g_ctx, p_device->getRSSI()));
      if( p_device->haveName() )
        JS_SetPropertyStr(g_ctx, item, "name", JS_NewString(g_ctx, p_device->getName().c_str()));
      if( p_device->haveServiceUUID() )
        JS_SetPropertyStr(g_ctx, item, "serviceUuid", JS_NewString(g_ctx, p_device->getServiceUUID().toString().c_str()));
      if( p_device->haveTXPower() )
        JS_SetPropertyStr(g_ctx, item, "txPower", JS_NewInt32(g_ctx, p_device->getTXPower()));

      JS_SetPropertyUint32(g_ctx, array, i, item);
    }
    ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
    JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &array);
    JS_FreeValue(g_ctx, array);
    JS_FreeValue(g_ctx, ret);

    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
    g_scanCompleted = false;
    isRunning = BLEBEACON_RUNNING_NONE;
  }
}

JsModuleEntry bledevice_module = {
  initializeModule_blecentral,
  addModule_blecentral,
  loopModule_blecentral,
  endModule_blecentral
};

static void taskServer(void *)
{
  BLEDevice::init("");
  g_pAdvertising = BLEDevice::getAdvertising();
  g_pScan = BLEDevice::getScan();

  g_pcallback = new MyScanCallbacks();
  g_pScan->setScanCallbacks(g_pcallback);

  Serial.println("[bledevice ready]");
//  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}

#endif