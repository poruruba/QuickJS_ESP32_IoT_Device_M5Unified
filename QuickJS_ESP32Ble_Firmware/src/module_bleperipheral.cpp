#include <Arduino.h>
#include "main_config.h"

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"

#ifdef _BLEPERIPHERAL_ENABLE_

#include <NimBLEDevice.h>
#include "module_utils.h"
#include "module_bleperipheral.h"

#define SERVICE_UUID  "83444e80-1f85-4471-a94c-2a5e96d9cdc8"
#define READ_CHARACTERISTIC_UUID "83444e81-1f85-4471-a94c-2a5e96d9cdc8"
#define WRITE_CHARACTERISTIC_UUID "83444e82-1f85-4471-a94c-2a5e96d9cdc8"

static NimBLEServer *g_pServer = NULL;
static NimBLECharacteristic *g_pCharacteristic_read = NULL;
static NimBLECharacteristic *g_pCharacteristic_write = NULL;
static NimBLEAdvertising *g_pAdvertising = NULL;
static bool g_isConnected = false;
static bool g_isNotifyEnabled = false;
static uint16_t g_connHandle;

typedef struct {
  uint8_t type;
  uint8_t *p_value;
  uint32_t length;
  std::string str;
  uint32_t passkey;
  bool is_authed;
  bool is_enced;
  bool is_bonded;
} BLEPERI_EVENT_INFO;
static std::vector<BLEPERI_EVENT_INFO> g_event_list;

#define BLEEVENT_TYPE_NONE            0
#define BLEEVENT_TYPE_CONNECT         1
#define BLEEVENT_TYPE_DISCONNECT      2
#define BLEEVENT_TYPE_CONNECT_FAILED  3
#define BLEEVENT_TYPE_WRITE           4
#define BLEEVENT_TYPE_READ            5
#define BLEEVENT_TYPE_NOTIFY          6
#define BLEEVENT_TYPE_AUTH_COMPLETE   7
#define BLEEVENT_TYPE_PASSKEY_DISPLAY 8
#define BLEEVENT_TYPE_PASSKEY_ENTRY   9

#define BLEBEACON_RUNNING_NONE      0
#define BLEBEACON_RUNNING_ADVERTISE 1

static int isRunning = BLEBEACON_RUNNING_NONE;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;

static void taskServer(void *);

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
//    Serial.println("BLEPeri.onWrite");
    if( g_callback_func == JS_UNDEFINED )
      return;
    BLEPERI_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_WRITE;
    std::string value = pCharacteristic->getValue();
    const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
    size_t len = value.length();

    info.length = value.length();
    info.p_value = (uint8_t*)malloc(info.length);
    memmove(info.p_value, data, info.length);

    g_event_list.push_back(info);
  }

  void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
//    Serial.println("BLEPeri.onSubscribe");
    if( subValue & 0x0001 )
      g_isNotifyEnabled = true;
    else
      g_isNotifyEnabled = false;
  }
};

class MyServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
//    Serial.println("BLEPeri.onConnect");
    if( g_callback_func == JS_UNDEFINED )
      return;

    BLEPERI_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_CONNECT;
    NimBLEAddress peerAddr = connInfo.getAddress();
    info.str = peerAddr.toString();
    info.p_value = NULL;

    g_event_list.push_back(info);

    g_isConnected = true;
    g_connHandle = connInfo.getConnHandle();
    g_pAdvertising->stop();
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
//    Serial.println("BLEPeri.onDisconnect");
    if( g_callback_func == JS_UNDEFINED )
      return;

    BLEPERI_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_DISCONNECT;
    info.p_value = NULL;
    g_event_list.push_back(info);

    g_isConnected = false;
    NimBLEDevice::startAdvertising();
  }

  uint32_t onPassKeyDisplay() override{
    uint32_t passkey = random(0, 1000000);

    if( g_callback_func != JS_UNDEFINED ){
      BLEPERI_EVENT_INFO info;
      info.type = BLEEVENT_TYPE_PASSKEY_DISPLAY;
      info.passkey = passkey;
      info.p_value = NULL;
      g_event_list.push_back(info);
    }

    return passkey;
  }

  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override{
//    Serial.println("BLEPeri.onAuthenticationComplete");
    if( g_callback_func == JS_UNDEFINED )
      return;

    BLEPERI_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_AUTH_COMPLETE;
    info.is_authed = connInfo.isAuthenticated();
    info.is_enced = connInfo.isEncrypted();
    info.is_bonded = connInfo.isBonded();
    info.p_value = NULL;
    g_event_list.push_back(info);
  }
};

static JSValue bleperipheral_startAdvertise(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLEBEACON_RUNNING_NONE)
    return JS_EXCEPTION;    

  g_pAdvertising->reset();
  g_pAdvertising->addServiceUUID(SERVICE_UUID);

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  if( argc >= 2 ){
    JSValue value;

    value = JS_GetPropertyStr(ctx, argv[1], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[1], "connectableMode");
    if( value != JS_UNDEFINED ){
      uint32_t connectableMode;
      JS_ToUint32(ctx, &connectableMode, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setConnectableMode(connectableMode);
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
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      g_pAdvertising->setManufacturerData(p_buffer, unit_num);
      JS_FreeValue(ctx, vbuffer);
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

  bool ret = g_pAdvertising->start(duration);
  isRunning = BLEBEACON_RUNNING_ADVERTISE;

  return JS_NewBool(ctx, ret);
}

static JSValue bleperipheral_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_callback_func != JS_UNDEFINED )
    JS_FreeValue(g_ctx, g_callback_func);

  g_ctx = ctx;
  g_callback_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_setReadValue(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[0], (void**)&p_buffer, &unit_size, &unit_num);
  if( vbuffer == JS_NULL ){
    return JS_EXCEPTION;
  }
  if( unit_size != 1 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  g_pCharacteristic_read->setValue(p_buffer, unit_num);
  JS_FreeValue(ctx, vbuffer);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_getWriteValue(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  std::string value = g_pCharacteristic_write->getValue();
  const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
  size_t len = value.length();

  return create_Uint8Array(ctx, data, len);
}

static JSValue bleperipheral_notify(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if (!g_isConnected || !g_isNotifyEnabled)
    return JS_NewBool(ctx, false);

  bool result = g_pCharacteristic_read->notify();

  return JS_NewBool(ctx, result);
}

static JSValue bleperipheral_startAdvertise2(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLEBEACON_RUNNING_NONE )
    return JS_EXCEPTION;    

  g_pAdvertising->reset();

  uint32_t duration = 0;
  JS_ToUint32(ctx, &duration, argv[0]);

  JSValue value;
  if( argc >= 2 ){
    value = JS_GetPropertyStr(ctx, argv[1], "connectableMode");
    if( value != JS_UNDEFINED ){
      uint32_t connectableMode;
      JS_ToUint32(ctx, &connectableMode, value);
      JS_FreeValue(ctx, value);
      g_pAdvertising->setConnectableMode(connectableMode);
    }
  }

  if( argc >= 3 ){
    NimBLEAdvertisementData advertisementData = NimBLEAdvertisementData();

    value = JS_GetPropertyStr(ctx, argv[2], "flags");
    if( value != JS_UNDEFINED ){
      uint32_t flags;
      JS_ToUint32(ctx, &flags, value);
      JS_FreeValue(ctx, value);
      advertisementData.setFlags(flags);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      advertisementData.setCompleteServices(uuid);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      advertisementData.setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      advertisementData.setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "manufacturerData");
    if( value != JS_UNDEFINED ){
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      advertisementData.setManufacturerData(p_buffer, unit_num);
      JS_FreeValue(ctx, vbuffer);
    }

    value = JS_GetPropertyStr(ctx, argv[2], "rawData");
    if( value != JS_UNDEFINED ){
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      advertisementData.addData(p_buffer, unit_num);
      free(p_buffer);
    }

    g_pAdvertising->setAdvertisementData(advertisementData);

    // advertisementData.setFlags(0x06);
    // advertisementData.setCompleteServices(BLEUUID("fe6f"));
    // unsigned char data2[] = {11, 0x16, 0x6f, 0xfe, 0x02, 0x01, 0x21, 0x3a, 0x72, 0x4f, 0x7f, 0x20 };
    // advertisementData.addData((char*)data2, sizeof(data2));
    // g_pAdvertising->setAdvertisementData(advertisementData);
  }

  if( argc >= 4 ){
    NimBLEAdvertisementData scanResponseData = NimBLEAdvertisementData();

    value = JS_GetPropertyStr(ctx, argv[3], "flags");
    if( value != JS_UNDEFINED ){
      uint32_t flags;
      JS_ToUint32(ctx, &flags, value);
      JS_FreeValue(ctx, value);
      scanResponseData.setFlags(flags);
    }

    value = JS_GetPropertyStr(ctx, argv[3], "serviceUuid");
    if( value != JS_UNDEFINED ){
      const char *serviceUuid = JS_ToCString(ctx, value);
      BLEUUID uuid = BLEUUID(serviceUuid);
      JS_FreeCString(ctx, serviceUuid);
      JS_FreeValue(ctx, value);
      if( uuid.bitSize() == 0 )
        return JS_EXCEPTION;

      scanResponseData.setCompleteServices(uuid);
    }

    value = JS_GetPropertyStr(ctx, argv[3], "name");
    if( value != JS_UNDEFINED ){
      const char *name = JS_ToCString(ctx, value);
      scanResponseData.setName(name);
      JS_FreeCString(ctx, name);
      JS_FreeValue(ctx, value);
    }

    value = JS_GetPropertyStr(ctx, argv[3], "appearance");
    if( value != JS_UNDEFINED ){
      uint32_t appearance;
      JS_ToUint32(ctx, &appearance, value);
      JS_FreeValue(ctx, value);
      scanResponseData.setAppearance(appearance);
    }

    value = JS_GetPropertyStr(ctx, argv[3], "manufacturerData");
    if( value != JS_UNDEFINED ){
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);

      scanResponseData.setManufacturerData((char*)p_buffer);
      JS_FreeValue(ctx, vbuffer);
    }

    value = JS_GetPropertyStr(ctx, argv[3], "rawData");
    if( value != JS_UNDEFINED ){
      uint32_t unit_num;
      uint8_t *p_buffer;
      JSValue vbuffer = from_Uint8Array(ctx, value, &p_buffer, &unit_num);
      if( JS_IsNull(vbuffer) ){
        JS_FreeValue(ctx, value);
        return JS_EXCEPTION;
      }
      JS_FreeValue(ctx, value);
      
      scanResponseData.addData(p_buffer, unit_num);
      free(p_buffer);
    }

    g_pAdvertising->setScanResponseData(scanResponseData);
  }

  g_pAdvertising->start();
  isRunning = BLEBEACON_RUNNING_ADVERTISE;

  return JS_UNDEFINED;
}

static JSValue bleperipheral_stopAdvertise(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning == BLEBEACON_RUNNING_ADVERTISE ){
    g_pAdvertising->stop();
    isRunning = BLEBEACON_RUNNING_NONE;
  }

  return JS_UNDEFINED;
}

static JSValue bleperipheral_isConnected(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, g_isConnected);
}

static JSValue bleperipheral_getMacAddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewString(ctx, BLEDevice::getAddress().toString().c_str());
}

static JSValue bleperipheral_setSecurity(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool bonding, mitm, sc;
  bonding = JS_ToBool(ctx, argv[0]);
  mitm = JS_ToBool(ctx, argv[1]);
  sc = JS_ToBool(ctx, argv[2]);
  uint32_t iocap;
  if( argc >= 4 )
    JS_ToUint32(ctx, &iocap, argv[3]);

  NimBLEDevice::setSecurityAuth(bonding, mitm, sc);
  if( argc >= 4 )
    NimBLEDevice::setSecurityIOCap(iocap);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_setPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
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

static JSValue bleperipheral_getPower(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int power = BLEDevice::getPower();

  return JS_NewInt32(ctx, power);
}

static JSValue bleperipheral_setMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t mtu;
  JS_ToUint32(ctx, &mtu, argv[0]);

  BLEDevice::setMTU(mtu);

  return JS_UNDEFINED;
}

static JSValue bleperipheral_getMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t mtu = BLEDevice::getMTU();

  return JS_NewUint32(ctx, mtu);
}

static const JSCFunctionListEntry bleperipheral_funcs[] = {
    JSCFunctionListEntry{
        "startAdvertise", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, bleperipheral_startAdvertise}
        }},
    JSCFunctionListEntry{
        "startAdvertise2", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, bleperipheral_startAdvertise2}
        }},
    JSCFunctionListEntry{
        "isConnected", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_isConnected}
        }},
    JSCFunctionListEntry{
        "setReadValue", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_setReadValue}
        }},
    JSCFunctionListEntry{
        "getWriteValue", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getWriteValue}
        }},
    JSCFunctionListEntry{
        "notify", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_notify}
        }},
    JSCFunctionListEntry{
        "setCallback", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bleperipheral_setCallback}
        }},
    JSCFunctionListEntry{
        "stopAdvertise", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_stopAdvertise}
        }},
    JSCFunctionListEntry{
        "getMacAddress", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getMacAddress}
        }},        
    JSCFunctionListEntry{
        "setSecurity", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, bleperipheral_setSecurity}
        }},        
    JSCFunctionListEntry{
        "setPower", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bleperipheral_setPower}
        }},        
    JSCFunctionListEntry{
        "getPower", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getPower}
        }},        
    JSCFunctionListEntry{
        "setMtu", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, bleperipheral_setMtu}
        }},        
    JSCFunctionListEntry{
        "getMtu", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, bleperipheral_getMtu}
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
};

JSModuleDef *addModule_bleperipheral(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "BlePeripheral", [](JSContext *ctx, JSModuleDef *m){
          return JS_SetModuleExportList(
                      ctx, m, bleperipheral_funcs,
                      sizeof(bleperipheral_funcs) / sizeof(JSCFunctionListEntry));
          });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, bleperipheral_funcs,
        sizeof(bleperipheral_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initializeModule_bleperipheral(void)
{
  taskServer(NULL);

  // BaseType_t ret = xTaskCreate(taskServer, "server", 4096, NULL, 5, NULL);
  // if( ret != pdPASS ){
  //   Serial.printf("xTaskCreate error=%d\n", ret);
  // }

  return 0;
}

void endModule_bleperipheral(void){
  if( isRunning == BLEBEACON_RUNNING_ADVERTISE ){
    g_pAdvertising->stop();
    isRunning = BLEBEACON_RUNNING_NONE;
  }

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  while(g_event_list.size() > 0){
    BLEPERI_EVENT_INFO info = (BLEPERI_EVENT_INFO)g_event_list.front();
    if( info.p_value != NULL )
      free(info.p_value);
    g_event_list.erase(g_event_list.begin());
  }
  
  if( g_isConnected ){
    g_pServer->disconnect(g_connHandle);
  }
  g_isNotifyEnabled = false;
  g_isConnected = false;

  // default
  g_pAdvertising->reset();
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(255);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);  
}

void loopModule_bleperipheral(void){
  if( g_callback_func != JS_UNDEFINED ){
    while(g_event_list.size() > 0){
      BLEPERI_EVENT_INFO info = (BLEPERI_EVENT_INFO)g_event_list.front();
      JSValue objs[2];
      switch(info.type){
        case BLEEVENT_TYPE_CONNECT: {
          objs[0] = JS_NewString(g_ctx, "connect");
          objs[1] = JS_NewObject(g_ctx);
          JS_SetPropertyStr(g_ctx, objs[1], "address", JS_NewString(g_ctx, info.str.c_str()));

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, objs[1]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        case BLEEVENT_TYPE_DISCONNECT: {
          objs[0] = JS_NewString(g_ctx, "disconnect");
          objs[1] = JS_UNDEFINED;

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        case BLEEVENT_TYPE_WRITE: {
          objs[0] = JS_NewString(g_ctx, "write");
          objs[1] = JS_NewObject(g_ctx);
          JS_SetPropertyStr(g_ctx, objs[1], "data", create_Uint8Array(g_ctx, info.p_value, info.length));
          free(info.p_value);

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, objs[1]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        case BLEEVENT_TYPE_PASSKEY_DISPLAY: {
          objs[0] = JS_NewString(g_ctx, "passkey_display");
          objs[1] = JS_NewObject(g_ctx);
          JS_SetPropertyStr(g_ctx, objs[1], "passkey", JS_NewUint32(g_ctx, info.passkey));

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, objs[1]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        case BLEEVENT_TYPE_AUTH_COMPLETE: {
          objs[0] = JS_NewString(g_ctx, "auth_complete");
          objs[1] = JS_NewObject(g_ctx);
          JS_SetPropertyStr(g_ctx, objs[1], "isAuthenticated", JS_NewBool(g_ctx, info.is_authed));
          JS_SetPropertyStr(g_ctx, objs[1], "isEncrypted", JS_NewBool(g_ctx, info.is_enced));
          JS_SetPropertyStr(g_ctx, objs[1], "isBonded", JS_NewBool(g_ctx, info.is_bonded));

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, objs[1]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        default:
          break;
      }
      g_event_list.erase(g_event_list.begin());
    }
  }
}

JsModuleEntry bleperipheral_module = {
  initializeModule_bleperipheral,
  addModule_bleperipheral,
  loopModule_bleperipheral,
  endModule_bleperipheral
};

static void taskServer(void *)
{
  BLEDevice::init("Esp32Ble");

  g_pServer = NimBLEDevice::createServer();
  g_pServer->setCallbacks(new MyServerCallbacks());
  NimBLEService *pService = g_pServer->createService(SERVICE_UUID);

  MyCharacteristicCallbacks *p_callbacks = new MyCharacteristicCallbacks();
  g_pCharacteristic_write = pService->createCharacteristic(WRITE_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  g_pCharacteristic_write->setCallbacks(p_callbacks);
  g_pCharacteristic_read = pService->createCharacteristic(READ_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::NOTIFY
  );
  g_pCharacteristic_read->setCallbacks(p_callbacks);
  pService->start();

  g_pAdvertising = NimBLEDevice::getAdvertising();

  Serial.printf("[bleperipheral ready] MAC=%s\n", BLEDevice::getAddress().toString().c_str());
//  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}

#endif