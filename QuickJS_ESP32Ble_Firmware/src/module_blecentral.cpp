#include <Arduino.h>
#include "main_config.h"

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_type.h"

#ifdef _BLECENTRAL_ENABLE_

#include <NimBLEDevice.h>
#include "module_utils.h"
#include "module_blecentral.h"

static NimBLEScan *g_pScan = NULL;
static NimBLEClient *g_pClient = NULL;

#define BLECENTRAL_RUNNING_NONE      0
#define BLECENTRAL_RUNNING_SCAN      1

static int isRunning = BLECENTRAL_RUNNING_NONE;

typedef struct{
  uint8_t type;
  std::string str;
  uint8_t *p_data;
  uint32_t length;
  bool is_authed;
  bool is_enced;
  bool is_bonded;
} BLECENT_EVENT_INFO;

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

static std::vector<BLECENT_EVENT_INFO> g_event_list;
static NimBLEScanCallbacks *g_pcallback = NULL;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static JSValue g_callback_scan_func = JS_UNDEFINED;
static bool g_scanCompleted = false;
static NimBLEConnInfo* g_pPendingConn = NULL;

static std::string g_connect_address;
static void task_connect(void *);
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
  g_pClient->cancelConnect();
  if (g_pClient->isConnected()) {
      g_pClient->setClientCallbacks(NULL);
      g_pClient->disconnect();
  }

  if( g_callback_scan_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_scan_func);
    g_callback_scan_func = JS_UNDEFINED;
    g_pScan->clearResults();
  }

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;

    while(g_event_list.size() > 0){
      BLECENT_EVENT_INFO info = (BLECENT_EVENT_INFO)g_event_list.front();
      if( info.p_data != NULL )
        free(info.p_data);
      g_event_list.erase(g_event_list.begin());
    }
  }
}

void myNotifyHandler(NimBLERemoteCharacteristic* pChar, uint8_t* data, size_t length, bool isNotify) {
//  Serial.print("Notify received: ");
  if( g_callback_func == JS_UNDEFINED )
    return;
  
  BLECENT_EVENT_INFO info;  
  info.type = BLEEVENT_TYPE_NOTIFY;
  info.str = pChar->getUUID().toString();
  info.p_data = (uint8_t*)malloc(length);
  if( info.p_data == NULL )
    return;
  memmove(info.p_data, data, length);
  info.length = length;

  g_event_list.push_back(info);
}

class MyClientCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient* pClient, int reason) override {
//    Serial.println("BleCent.onDisconnect");
    g_pClient->setClientCallbacks(NULL);

    if( g_callback_func == JS_UNDEFINED )
      return;

    BLECENT_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_DISCONNECT;
    info.p_data = NULL;
    g_event_list.push_back(info);
  }

  void onPassKeyEntry(NimBLEConnInfo& connInfo) override {
//    Serial.println("BleCent.onPassKeyEntry");
    if( g_callback_func == JS_UNDEFINED )
      return;

    if( g_pPendingConn != NULL )
      delete g_pPendingConn;
    g_pPendingConn = new NimBLEConnInfo(connInfo);

    BLECENT_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_PASSKEY_ENTRY;
    info.p_data = NULL;
    g_event_list.push_back(info);    
  }

  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
//    Serial.println("BleCent.onAuthenticationComplete");
    if( g_callback_func == JS_UNDEFINED )
      return;

    BLECENT_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_AUTH_COMPLETE;
    info.is_authed = connInfo.isAuthenticated();
    info.is_enced = connInfo.isEncrypted();
    info.is_bonded = connInfo.isBonded();
    info.p_data = NULL;
    g_event_list.push_back(info);
  }
};
static MyClientCallbacks *g_pClientCallbacks = NULL;

class MyScanCallbacks: public NimBLEScanCallbacks
{
  // void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
  // }
  void onScanEnd(const NimBLEScanResults& scanResults, int reason) override{
    g_scanCompleted = true;
  }
};

void task_connect(void* param)
{
  bool secureConnect = (bool)param;
  NimBLEAddress address(g_connect_address, BLE_ADDR_PUBLIC);
  if (g_pClient->connect(address)) {
    Serial.println("Connected from separate thread");
    g_pClient->setClientCallbacks(g_pClientCallbacks);

    BLECENT_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_CONNECT;
    info.str = g_connect_address;
    info.p_data = NULL;
    g_event_list.push_back(info);
    if( secureConnect )
      g_pClient->secureConnection(true);
  } else {
    Serial.println("Connection failed");
    g_pClient->setClientCallbacks(NULL);
    g_pClient->cancelConnect();

    BLECENT_EVENT_INFO info;
    info.type = BLEEVENT_TYPE_CONNECT_FAILED;
    info.p_data = NULL;
    g_event_list.push_back(info);
  }

  vTaskDelete(NULL); // タスク終了
}

static JSValue blecentral_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if (g_pClient->isConnected())
      g_pClient->disconnect();

  const char *p_address = JS_ToCString(ctx, argv[0]);
  g_connect_address = p_address;
  JS_FreeCString(ctx, p_address);

  bool secureConnect = false;
  if( argc >= 2 )
    secureConnect = JS_ToBool(ctx, argv[1]);

  BaseType_t result = xTaskCreate(task_connect, "server_connect", 4092, (void*)secureConnect, 5, NULL);
  if( result != pdPASS ){
    Serial.printf("xTaskCreate error=%d\n", result);
  }

  return JS_UNDEFINED;

  // NimBLEClient *pClient = NimBLEDevice::createClient();
  // NimBLEAddress address(g_connect_address, BLE_ADDR_PUBLIC);
  // bool result = pClient->connect(address);
  // if( result ){
  //   g_pClient = pClient;
  // }else{
  //   NimBLEDevice::deleteClient(pClient);
  // }
  // return JS_NewBool(ctx, result);
}

static JSValue blecentral_disconnect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !g_pClient->isConnected() )
    return JS_UNDEFINED;

  g_pClient->setClientCallbacks(NULL);
  g_pClient->disconnect();

  return JS_UNDEFINED;
}

static JSValue blecentral_listPrimaryService(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !g_pClient->isConnected() )
    return JS_EXCEPTION;

    std::vector<NimBLERemoteService*> services = g_pClient->getServices();
    JSValue array = JS_NewArray(ctx);
    for( int i = 0 ; i < services.size() ; i++ ){
      NimBLERemoteService* pService = services[i];
      JS_SetPropertyUint32(ctx, array, i, JS_NewString(ctx, pService->getUUID().toString().c_str()));
    }

    return array;
}

static JSValue blecentral_listCharacteristic(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !g_pClient->isConnected() )
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

static JSValue blecentral_write(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !g_pClient->isConnected() )
    return JS_EXCEPTION;

  const char* serviceUuid = JS_ToCString(ctx, argv[0]);
  if( serviceUuid == NULL )
    return JS_EXCEPTION;
  const char* characteristicUuid = JS_ToCString(ctx, argv[1]);
  if( characteristicUuid == NULL ){
    JS_FreeCString(ctx, serviceUuid);
    return JS_EXCEPTION;
  }

  uint32_t unit_num;
  uint8_t *p_buffer;
  JSValue vbuffer = from_Uint8Array(ctx, argv[2], &p_buffer, &unit_num);
  if( JS_IsNull(vbuffer) ){
    JS_FreeCString(ctx, serviceUuid);
    JS_FreeCString(ctx, characteristicUuid);
    return JS_EXCEPTION;
  }

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

  if( !pChar->canWriteNoResponse() && !pChar->canWrite() )
    return JS_EXCEPTION;

  bool result = pChar->writeValue(p_buffer, unit_num, pChar->canWrite());
  JS_FreeValue(ctx, vbuffer);

  return JS_NewBool(ctx, result);
}

static JSValue blecentral_read(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !g_pClient->isConnected() )
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

  return create_Uint8Array(ctx, data, len);
}

static JSValue blecentral_subscribe(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( !g_pClient->isConnected() )
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

  bool result;
  if( magic == 0 )
    result = pChar->subscribe(true, myNotifyHandler, false);
  else
    result = pChar->unsubscribe(false);

  return JS_NewBool(ctx, result);
}

static JSValue blecentral_isConnected(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  return JS_NewBool(ctx, g_pClient->isConnected());
}

static JSValue blecentral_startScan(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isRunning != BLECENTRAL_RUNNING_NONE)
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
  g_callback_scan_func = JS_DupValue(g_ctx, argv[2]);

  g_scanCompleted = false;
  g_pScan->clearResults();

  bool ret = g_pScan->start(duration);
  isRunning = BLECENTRAL_RUNNING_SCAN;

  return JS_NewBool(ctx, ret);
}

static JSValue blecentral_setMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t mtu;
  JS_ToUint32(ctx, &mtu, argv[0]);

  bool ret = BLEDevice::setMTU(mtu);

  return JS_NewBool(ctx, ret);
}

static JSValue blecentral_getMtu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t mtu = BLEDevice::getMTU();

  return JS_NewUint32(ctx, mtu);
}

static JSValue blecentral_injectPassKey(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_pPendingConn == NULL )
    return JS_EXCEPTION;

  uint32_t passkey;
  JS_ToUint32(ctx, &passkey, argv[0]);

  bool ret = NimBLEDevice::injectPassKey(*g_pPendingConn, passkey);

  return JS_NewBool(ctx, ret);
}

static void blecentral_stopScan(void){
  if( isRunning == BLECENTRAL_RUNNING_SCAN ){
    g_pScan->stop();
    g_pScan->clearResults();
    g_scanCompleted = false;
    isRunning = BLECENTRAL_RUNNING_NONE;
  }

  if( g_callback_scan_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_scan_func);
    g_callback_scan_func = JS_UNDEFINED;
  }
}

static JSValue blecentral_stopScan(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  blecentral_stopScan();

  return JS_UNDEFINED;
}

static JSValue blecentral_getMacAddress(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewString(ctx, BLEDevice::getAddress().toString().c_str());
}

static JSValue blecentral_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_callback_func != JS_UNDEFINED )
    JS_FreeValue(g_ctx, g_callback_func);

  g_ctx = ctx;
  g_callback_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static JSValue blecentral_setSecurity(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool bonding, mitm, sc;
  bonding = JS_ToBool(ctx, argv[0]);
  mitm = JS_ToBool(ctx, argv[1]);
  sc = JS_ToBool(ctx, argv[2]);
  uint32_t iocap;
  if( argc >= 4)
    JS_ToUint32(ctx, &iocap, argv[3]);

  NimBLEDevice::setSecurityAuth(bonding, mitm, sc);
  if( argc >= 4)
    NimBLEDevice::setSecurityIOCap(iocap);

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry blecentral_funcs[] = {
    JSCFunctionListEntry{
        "connect", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, blecentral_connect}
        }},
    JSCFunctionListEntry{
        "disconnect", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, blecentral_disconnect}
        }},
    JSCFunctionListEntry{
        "listPrimaryService", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, blecentral_listPrimaryService}
        }},
    JSCFunctionListEntry{
        "listCharacteristic", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, blecentral_listCharacteristic}
        }},
    JSCFunctionListEntry{
        "write", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, blecentral_write}
        }},
    JSCFunctionListEntry{
        "read", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, blecentral_read}
        }},
    JSCFunctionListEntry{
        "subscribe", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic_magic, {generic_magic : blecentral_subscribe}}
        }},
    JSCFunctionListEntry{
        "isConnected", 0, JS_DEF_CFUNC, 1, {
          func : {2, JS_CFUNC_generic_magic, {generic_magic : blecentral_isConnected}}
        }},
    JSCFunctionListEntry{
        "unsubscribe", 0, JS_DEF_CFUNC, 1, {
          func : {2, JS_CFUNC_generic_magic, {generic_magic : blecentral_subscribe}}
        }},
    JSCFunctionListEntry{
        "setCallback", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, blecentral_setCallback}
        }},
    JSCFunctionListEntry{
        "startScan", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, blecentral_startScan}
        }},
    JSCFunctionListEntry{
        "stopScan", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, blecentral_stopScan}
        }},
    JSCFunctionListEntry{
        "setMtu", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, blecentral_setMtu}
        }},
    JSCFunctionListEntry{
        "getMtu", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, blecentral_getMtu}
        }},
    JSCFunctionListEntry{
        "getMacAddress", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, blecentral_getMacAddress}
        }},
    JSCFunctionListEntry{
        "setSecurity", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, blecentral_setSecurity}
        }},
    JSCFunctionListEntry{
        "injectPassKey", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, blecentral_injectPassKey}
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
  mod = JS_NewCModule(ctx, "BleCentral", [](JSContext *ctx, JSModuleDef *m){
          return JS_SetModuleExportList(
                      ctx, m, blecentral_funcs,
                      sizeof(blecentral_funcs) / sizeof(JSCFunctionListEntry));
          });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, blecentral_funcs,
        sizeof(blecentral_funcs) / sizeof(JSCFunctionListEntry));
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

void endModule_blecentral(void)
{
  blecentral_stopScan();

  if( g_callback_scan_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_scan_func);
    g_callback_scan_func = JS_UNDEFINED;
    g_pScan->clearResults();
    g_scanCompleted = false;
  }

  if (g_pClient->isConnected()) {
      g_pClient->setClientCallbacks(NULL);
      g_pClient->disconnect();
  }

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;

    while(g_event_list.size() > 0){
      BLECENT_EVENT_INFO info = (BLECENT_EVENT_INFO)g_event_list.front();
      if( info.p_data != NULL )
        free(info.p_data);
      g_event_list.erase(g_event_list.begin());
    }
  }

  // default
  NimBLEDevice::setMTU(255);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
}

void loopModule_blecentral(void){
  if( g_callback_func != JS_UNDEFINED ){
    while(g_event_list.size() > 0){
      BLECENT_EVENT_INFO info = (BLECENT_EVENT_INFO)g_event_list.front();
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
        case BLEEVENT_TYPE_CONNECT_FAILED: {
          objs[0] = JS_NewString(g_ctx, "connect_failed");
          objs[1] = JS_UNDEFINED;

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
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
        case BLEEVENT_TYPE_NOTIFY: {
          objs[0] = JS_NewString(g_ctx, "notify");
          objs[1] = JS_NewObject(g_ctx);
          JS_SetPropertyStr(g_ctx, objs[1], "characteristic", JS_NewString(g_ctx, info.str.c_str()));
          JS_SetPropertyStr(g_ctx, objs[1], "data", create_Uint8Array(g_ctx, info.p_data, info.length));
          free(info.p_data);

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
          JS_FreeValue(g_ctx, objs[1]);
          JS_FreeValue(g_ctx, ret);
          break;
        }
        case BLEEVENT_TYPE_PASSKEY_ENTRY: {
          objs[0] = JS_NewString(g_ctx, "passkey_entry");
          objs[1] = JS_UNDEFINED;

          ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
          JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
          JS_FreeValue(g_ctx, objs[0]);
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
        default: {
          break;
        }
      }
      g_event_list.erase(g_event_list.begin());
    }
  }

  if( g_scanCompleted && g_callback_scan_func != JS_UNDEFINED ){
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
    g_pScan->clearResults();

    ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
    JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_scan_func, g_callback_scan_func, 1, &array);
    JS_FreeValue(g_ctx, array);
    JS_FreeValue(g_ctx, ret);

    JS_FreeValue(g_ctx, g_callback_scan_func);
    g_callback_scan_func = JS_UNDEFINED;
    g_scanCompleted = false;
    isRunning = BLECENTRAL_RUNNING_NONE;
  }
}

JsModuleEntry blecentral_module = {
  initializeModule_blecentral,
  addModule_blecentral,
  loopModule_blecentral,
  endModule_blecentral
};

static void taskServer(void *)
{
  BLEDevice::init("");
  g_pScan = NimBLEDevice::getScan();

  g_pcallback = new MyScanCallbacks();
  g_pScan->setScanCallbacks(g_pcallback);

  g_pClientCallbacks = new MyClientCallbacks();
  g_pClient = NimBLEDevice::createClient();

  Serial.printf("[blecentral ready] MAC=%s\n", BLEDevice::getAddress().toString().c_str());
//  vTaskDelay(portMAX_DELAY); //delay(portMAX_DELAY);
}

#endif