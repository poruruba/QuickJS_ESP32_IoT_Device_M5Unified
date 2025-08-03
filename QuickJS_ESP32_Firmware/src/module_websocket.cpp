#include <Arduino.h>
#include "main_config.h"

#ifdef _WEBSOCKET_ENABLE_

#include "quickjs.h"
#include "module_type.h"
#include "module_websocket.h"
#include "quickjs_esp32.h"
#include <ESPAsyncWebServer.h>

#include <vector>

static std::vector<AsyncWebSocketClient *> client_list;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static uint32_t latest_connect_client_id = 0;
static uint32_t latest_disconnect_client_id = 0;
static uint32_t latest_receive_client_id = 0;
static char* gp_latest_text = NULL;
static uint32_t g_total_length = 0;
static bool g_received = false;

void onWebsocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
  if( g_ctx == NULL )
    return;

  if(type == WS_EVT_CONNECT){
//    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
//    client->ping();

    client_list.push_back(client);
    latest_connect_client_id = client->id();
  } else

  if(type == WS_EVT_DISCONNECT){
//    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());

    for (auto itr = client_list.begin(); itr != client_list.end(); itr++){
      if( (*itr) == client ){
        client_list.erase(itr);
        break;
      }
    }

    latest_disconnect_client_id = client->id();
  } else

  if(type == WS_EVT_ERROR){
//    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else

  if(type == WS_EVT_DATA){
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if( info->opcode != WS_TEXT )
      return;

    if( info->index == 0){
      g_received = false;
      if( gp_latest_text != NULL )
        free(gp_latest_text);
      g_total_length = info->len;
      gp_latest_text = (char*)malloc(g_total_length + 1);
      if( gp_latest_text == NULL )
        return;
      latest_receive_client_id = client->id();
    }
    if( latest_receive_client_id != client->id() )
      return;

    if( info->index + len > g_total_length )
      return;
    memmove(&gp_latest_text[info->index], data, len);
    if( info->final ){
      gp_latest_text[g_total_length] = '\0';
      g_received = true;
    }
  }
}

static JSValue websocket_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_callback_func != JS_UNDEFINED )
    JS_FreeValue(g_ctx, g_callback_func);

  g_ctx = ctx;
  g_callback_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static JSValue websocket_send(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t client_id;
  JS_ToUint32(ctx, &client_id, argv[0]);
  const char* p_payload = JS_ToCString(ctx, argv[1]);

  bool found = false;
  for (auto itr = client_list.begin(); itr != client_list.end(); itr++){
    AsyncWebSocketClient *client = (*itr);
    if( client->id() == client_id ){
      client->text(p_payload);
      found = true;
      break;
    }
  }
  JS_FreeCString(ctx, p_payload);
  if( !found )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue websocket_getClientIdList(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  JSValue jsArray = JS_NewArray(ctx);
  int index = 0;
  for (auto itr = client_list.begin(); itr != client_list.end(); itr++){
    AsyncWebSocketClient *client = (*itr);
    JS_SetPropertyUint32(ctx, jsArray, index, JS_NewUint32(ctx, client->id()));
    index++;
  }

  return jsArray;
}

static JSValue websocket_close(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t client_id;
  JS_ToUint32(ctx, &client_id, argv[0]);

  bool found = false;
  for (auto itr = client_list.begin(); itr != client_list.end(); itr++){
    if( (*itr)->id() == client_id ){
      client_list.erase(itr);
      found = true;
      break;
    }
  }
  if( !found )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry websocket_funcs[] = {
    JSCFunctionListEntry{"setCallback", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, websocket_setCallback}
                         }},
    JSCFunctionListEntry{"send", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, websocket_send}
                         }},
    JSCFunctionListEntry{"getClientIdList", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, websocket_getClientIdList}
                         }},
    JSCFunctionListEntry{"close", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, websocket_close}
                         }},
};

JSModuleDef *addModule_websocket(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Websocket", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, websocket_funcs,
              sizeof(websocket_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, websocket_funcs,
        sizeof(websocket_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_websocket(void)
{
  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  g_ctx = NULL;

  for (auto itr: client_list)
    (*itr).close();
  client_list.clear();

  latest_connect_client_id = 0;
  latest_disconnect_client_id = 0;
  latest_receive_client_id = 0;
  if( gp_latest_text != NULL ){
    free(gp_latest_text);
    gp_latest_text = NULL;
  }
  g_received = false;
}

void loopModule_websocket(void){
  if( g_ctx != NULL ){
    if( latest_connect_client_id != 0){
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "connected"));
        JS_SetPropertyStr(g_ctx, obj, "client_id", JS_NewUint32(g_ctx, latest_connect_client_id));
        latest_connect_client_id = 0;

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }
    if( latest_disconnect_client_id != 0){
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "disconnected"));
        JS_SetPropertyStr(g_ctx, obj, "client_id", JS_NewUint32(g_ctx, latest_disconnect_client_id));
        latest_disconnect_client_id = 0;

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }
    if( g_received ){
        JSValue obj = JS_NewObject(g_ctx);
        JS_SetPropertyStr(g_ctx, obj, "type", JS_NewString(g_ctx, "received"));
        JS_SetPropertyStr(g_ctx, obj, "client_id", JS_NewUint32(g_ctx, latest_receive_client_id));
        JS_SetPropertyStr(g_ctx, obj, "payload", JS_NewString(g_ctx, (const char *)gp_latest_text));
        free(gp_latest_text);
        gp_latest_text = NULL;
        latest_receive_client_id = 0;
        g_received = false;

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
        JS_FreeValue(g_ctx, obj);
        JS_FreeValue(g_ctx, ret);
    }
  }
}

JsModuleEntry websocket_module = {
  NULL,
  addModule_websocket,
  loopModule_websocket,
  endModule_websocket
};

#endif
