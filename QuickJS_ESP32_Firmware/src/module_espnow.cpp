#include <Arduino.h>
#include "main_config.h"

#ifdef _ESPNOW_ENABLE_

#include <esp_now.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_espnow.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "wifi_utils.h"

static JSValue g_callback_func = JS_UNDEFINED;

#define ESPNOW_EVENT_TYPE_SEND  0
#define ESPNOW_EVENT_TYPE_RECV  1

typedef struct {
  uint8_t type;
  uint8_t macaddress[6];
  int32_t send_status;
  char *p_recv_data;
} ESPNOW_EVENT_INFO;
static std::vector<ESPNOW_EVENT_INFO> g_event_list;

static JSContext *g_ctx;
static bool isInitialized = false;

static void espnow_OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  Serial.println("espnow_OnDataSend");
  ESPNOW_EVENT_INFO info;
  info.type = ESPNOW_EVENT_TYPE_SEND;
  memmove(info.macaddress, mac_addr, 6);

  info.send_status = status;
  info.p_recv_data = NULL;

  g_event_list.push_back(info);
}

static void espnow_OnDataRecv(const uint8_t *mac_addr, const uint8_t *recvData, int len) {
//  Serial.println("espnow_OnDataRecv");
  ESPNOW_EVENT_INFO info;
  info.type = ESPNOW_EVENT_TYPE_RECV;
  memmove(info.macaddress, mac_addr, 6);

  info.p_recv_data = (char*)malloc(len + 1);
  if(info.p_recv_data == NULL )
    return;
  memmove(info.p_recv_data, recvData, len);
  info.p_recv_data[len] = '\0';

  g_event_list.push_back(info);
}

#if defined(ARDUINO_ESP32C6_DEV)
static void espnow_OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *recvData, int len) {
//  espnow_OnDataRecv(info->src_addr, recvData, len);
}
#endif

static JSValue espnow_send(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  JSValue jv;

  uint32_t macaddress_len;
  jv = JS_GetPropertyStr(ctx, argv[0], "length");
  JS_ToUint32(ctx, &macaddress_len, jv);
  JS_FreeValue(ctx, jv);
  if( macaddress_len < 6 )
    return JS_EXCEPTION;

  uint8_t macaddress[6];
  for (uint32_t i = 0; i < sizeof(macaddress); i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[0], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    macaddress[i] = (uint8_t)value;
  }

  const char *data = JS_ToCString(ctx, argv[1]);
  if( data == NULL )
    return JS_EXCEPTION;
  esp_err_t result = esp_now_send(macaddress, (uint8_t*)data, strlen(data));
  JS_FreeCString(ctx, data);
  if( result != ESP_OK)
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_ctx = ctx;

  if(g_callback_func != JS_UNDEFINED){
    JS_FreeValue(ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  g_callback_func = JS_DupValue(ctx, argv[0]);
  if( g_callback_func == JS_UNDEFINED )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_addPeer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  JSValue jv;
  uint32_t macaddress_len;

  jv = JS_GetPropertyStr(ctx, argv[0], "length");
  JS_ToUint32(ctx, &macaddress_len, jv);
  JS_FreeValue(ctx, jv);
  if( macaddress_len < 6 )
    return JS_EXCEPTION;

  esp_now_peer_info_t peerInfo = {};
  for (int i = 0; i < 6; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[0], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    peerInfo.peer_addr[i] = (uint8_t)value;
  }
  peerInfo.channel = 0;
  bool encrypt = false;
  if( argc >= 2 )
    encrypt = JS_ToBool(ctx, argv[1]);
  peerInfo.encrypt = encrypt;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_deletePeer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  JSValue jv;
  uint32_t macaddress_len;

  jv = JS_GetPropertyStr(ctx, argv[0], "length");
  JS_ToUint32(ctx, &macaddress_len, jv);
  JS_FreeValue(ctx, jv);
  if( macaddress_len < 6 )
    return JS_EXCEPTION;

  uint8_t macaddress[6];
  for (int i = 0; i < sizeof(macaddress); i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[0], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    macaddress[i] = (uint8_t)value;
  }
  
  if (esp_now_del_peer(macaddress) != ESP_OK)
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_clearPeer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;
    
  esp_now_peer_info_t peer;
  for (esp_err_t e = esp_now_fetch_peer(true, &peer); e == ESP_OK; e = esp_now_fetch_peer(false, &peer)) {
    esp_now_del_peer(peer.peer_addr);
  }

  return JS_UNDEFINED;
}

static JSValue espnow_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isInitialized )
    return JS_EXCEPTION;

  if( argc >= 1 ){
    if( wifi_is_connected() )
      return JS_EXCEPTION;
    uint32_t channel;
    JS_ToUint32(ctx, &channel, argv[0]);
    WiFi.mode(WIFI_STA);
    long ret = wifi_change_channel(channel);
    if( ret != 0 )
      return JS_EXCEPTION;
  }
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return JS_EXCEPTION;
  }
  esp_wifi_set_ps(WIFI_PS_NONE);

  esp_now_register_send_cb(espnow_OnDataSend);
  esp_now_register_recv_cb(espnow_OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  for (int i = 0; i < 6; i++)
    peerInfo.peer_addr[i] = 0xff;
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);

  isInitialized = true;

  return JS_UNDEFINED;
}

static JSValue espnow_end(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_UNDEFINED;

  esp_now_unregister_send_cb();
  esp_now_unregister_recv_cb();

  esp_now_peer_info_t peer;
  for (esp_err_t e = esp_now_fetch_peer(true, &peer); e == ESP_OK; e = esp_now_fetch_peer(false, &peer)) {
    esp_now_del_peer(peer.peer_addr);
  }

  while(g_event_list.size() > 0){
    ESPNOW_EVENT_INFO info = g_event_list.front();
    if( info.p_recv_data != NULL )
      free(info.p_recv_data);
    g_event_list.erase(g_event_list.begin());
  }

  esp_now_deinit();
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  isInitialized = false;

  return JS_UNDEFINED;
}

static JSValue espnow_isInitialized(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, isInitialized);
}

static const JSCFunctionListEntry espnow_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, espnow_begin}
        }},
    JSCFunctionListEntry{
        "end", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, espnow_end}
        }},
    JSCFunctionListEntry{
        "isInitialized", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, espnow_isInitialized}
        }},
    JSCFunctionListEntry{
        "send", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, espnow_send}
        }},
    JSCFunctionListEntry{
        "setCallback", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, espnow_setCallback}
        }},
    JSCFunctionListEntry{
        "addPeer", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, espnow_addPeer}
        }},
    JSCFunctionListEntry{
        "deletePeer", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, espnow_deletePeer}
        }},
    JSCFunctionListEntry{
        "clearPeer", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, espnow_clearPeer}
        }},
};

JSModuleDef *addModule_espnow(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "EspNow", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, espnow_funcs,
                            sizeof(espnow_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, espnow_funcs,
        sizeof(espnow_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void loopModule_espnow(void){
  if( g_ctx != NULL && g_callback_func != JS_UNDEFINED ){
    while(g_event_list.size() > 0){
      ESPNOW_EVENT_INFO info = g_event_list.front();
      if( info.type == ESPNOW_EVENT_TYPE_SEND ){
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "send"));
        JSValue jsArray = JS_NewArray(g_ctx);
        for (int i = 0; i < 6; i++)
          JS_SetPropertyUint32(g_ctx, jsArray, i, JS_NewInt32(g_ctx, info.macaddress[i]));
        JS_SetPropertyStr(g_ctx, obj, "macaddress", jsArray);
        JS_SetPropertyStr(g_ctx, obj, "status", JS_NewInt32(g_ctx, info.send_status));

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
      }else if( info.type == ESPNOW_EVENT_TYPE_RECV ){
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "recv"));
        JSValue jsArray = JS_NewArray(g_ctx);
        for (int i = 0; i < 6; i++)
          JS_SetPropertyUint32(g_ctx, jsArray, i, JS_NewInt32(g_ctx, info.macaddress[i]));
        JS_SetPropertyStr(g_ctx, obj, "macaddress", jsArray);
        JS_SetPropertyStr(g_ctx, obj, "data", JS_NewString(g_ctx, info.p_recv_data));
        free(info.p_recv_data);

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
      }

      g_event_list.erase(g_event_list.begin());
    }
  }
}

void endModule_espnow(void){
  if( isInitialized ){
    esp_now_unregister_send_cb();
    esp_now_unregister_recv_cb();
    esp_now_peer_info_t peer;
    for (esp_err_t e = esp_now_fetch_peer(true, &peer); e == ESP_OK; e = esp_now_fetch_peer(false, &peer)) {
      esp_now_del_peer(peer.peer_addr);
    }
    esp_now_deinit();
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    isInitialized = false;
  }

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  while(g_event_list.size() > 0){
    ESPNOW_EVENT_INFO info = g_event_list.front();
    if( info.p_recv_data != NULL )
      free(info.p_recv_data);
    g_event_list.erase(g_event_list.begin());
  }
  g_ctx = NULL;
}

JsModuleEntry espnow_module = {
  "EspNow",
  NULL,
  addModule_espnow,
  loopModule_espnow,
  endModule_espnow
};

#endif
