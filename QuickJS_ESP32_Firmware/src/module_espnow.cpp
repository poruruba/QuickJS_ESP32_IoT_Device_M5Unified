#include <Arduino.h>
#include "main_config.h"

#ifdef _ESPNOW_ENABLE_

#include <WiFi.h>
#include <esp_now.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_espnow.h"

static JSValue g_callback_func = JS_UNDEFINED;

static JSContext *g_ctx;
static esp_now_peer_info_t peerInfo;
static bool g_send_cb_fire = false;
static bool g_recv_cb_fire = false;
static int32_t g_send_status;
static uint8_t g_send_macaddress[6];
static uint8_t g_recv_macaddress[6];
static char *gp_recv_data = NULL;
static bool isInitialized = false;

void endModule_espnow(void);

static void espnow_OnDataSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
  memmove(g_send_macaddress, mac_addr, 6);
  g_send_status = status;
  g_send_cb_fire = true;
}

static void espnow_OnDataRecv(const uint8_t *mac_addr, const uint8_t *recvData, int len) {
  if( gp_recv_data != NULL ){
    free(gp_recv_data);
    gp_recv_data = NULL;
  }

  gp_recv_data = (char*)malloc(len + 1);
  if(gp_recv_data == NULL )
    return;
  
  memmove(gp_recv_data, recvData, len);
  gp_recv_data[len] = '\0';
  memmove(g_recv_macaddress, mac_addr, 6);
  g_recv_cb_fire = true;
}

static JSValue espnow_send(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
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
  if( result != ESP_OK){
    JS_FreeCString(ctx, data);
    return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static JSValue espnow_setCallback(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  g_ctx = ctx;

  esp_now_unregister_send_cb();
  esp_now_unregister_recv_cb();

  if(g_callback_func != JS_UNDEFINED){
    JS_FreeValue(ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  g_callback_func = JS_DupValue(ctx, argv[0]);
  if( g_callback_func != JS_UNDEFINED ){
    esp_now_register_send_cb(espnow_OnDataSend);
    esp_now_register_recv_cb(espnow_OnDataRecv);
  }

  return JS_UNDEFINED;
}

static JSValue espnow_addPeer(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  JSValue jv;
  uint32_t macaddress_len;

  jv = JS_GetPropertyStr(ctx, argv[0], "length");
  JS_ToUint32(ctx, &macaddress_len, jv);
  JS_FreeValue(ctx, jv);
  if( macaddress_len < 6 )
    return JS_EXCEPTION;

  for (uint32_t i = 0; i < sizeof(peerInfo.peer_addr); i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[0], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    peerInfo.peer_addr[i] = (uint8_t)value;
  }
  uint32_t val;
  JS_ToUint32(ctx, &val, argv[1]);
  peerInfo.channel = (uint8_t)val;
  int encrypt = JS_ToBool(ctx, argv[2]);
  peerInfo.encrypt = (encrypt != 0);
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_deletePeer(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
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
  
  if (esp_now_del_peer(macaddress) != ESP_OK)
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue espnow_clearPeer(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
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

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return JS_EXCEPTION;
  }
  isInitialized = true;

  return JS_UNDEFINED;
}

static JSValue espnow_end(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  endModule_espnow();

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
          func : {3, JS_CFUNC_generic, espnow_addPeer}
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
  if( g_ctx != NULL ){
    if( g_send_cb_fire ){
        Serial.println("g_send_cb_fire");

        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "send"));
        JSValue jsArray = JS_NewArray(g_ctx);
        for (int i = 0; i < 6; i++)
          JS_SetPropertyUint32(g_ctx, jsArray, i, JS_NewInt32(g_ctx, g_send_macaddress[i]));
        JS_SetPropertyStr(g_ctx, obj, "macaddress", jsArray);
        JS_SetPropertyStr(g_ctx, obj, "status", JS_NewInt32(g_ctx, g_send_status));
        g_send_cb_fire = false;

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }

    if( g_recv_cb_fire ){
        Serial.println("g_recv_cb_fire");

        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "recv"));
        JSValue jsArray = JS_NewArray(g_ctx);
        for (int i = 0; i < 6; i++)
          JS_SetPropertyUint32(g_ctx, jsArray, i, JS_NewInt32(g_ctx, g_recv_macaddress[i]));
        JS_SetPropertyStr(g_ctx, obj, "macaddress", jsArray);
        JS_SetPropertyStr(g_ctx, obj, "data", JS_NewString(g_ctx, gp_recv_data));
        free(gp_recv_data);
        gp_recv_data = NULL;
        g_recv_cb_fire = false;

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }
  }
}

void endModule_espnow(void){
  esp_now_peer_info_t peer;
  for (esp_err_t e = esp_now_fetch_peer(true, &peer); e == ESP_OK; e = esp_now_fetch_peer(false, &peer)) {
    esp_now_del_peer(peer.peer_addr);
  }

  if( isInitialized ){
    if( g_ctx != NULL ){
      if( g_callback_func != JS_UNDEFINED ){
        esp_now_unregister_send_cb();
        esp_now_unregister_recv_cb();
        JS_FreeValue(g_ctx, g_callback_func);
        g_callback_func = JS_UNDEFINED;
        if( gp_recv_data != NULL ){
          free(gp_recv_data);
          gp_recv_data = NULL;
        }
        g_send_cb_fire = false;
        g_recv_cb_fire = false;
      }
      g_ctx = NULL;
    }

    esp_now_deinit();
    isInitialized = false;
  }
}

JsModuleEntry espnow_module = {
  NULL,
  addModule_espnow,
  loopModule_espnow,
  endModule_espnow
};

#endif
