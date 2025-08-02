#include <Arduino.h>
#include "main_config.h"

#ifdef _WEBSOCKET_CLIENT_ENABLE_

#include "quickjs.h"
#include "module_type.h"
#include "module_websocket_client.h"
#include "quickjs_esp32.h"
#include <WebSocketsClient.h>

static WebSocketsClient webSocketClient;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static uint8_t g_type = 0;
static char *g_payload = NULL;

void onWebsocketsEvent(WStype_t type, uint8_t * payload, size_t length)
{
  Serial.printf("WebSocket Event: %d\n", type);
  if( g_ctx == NULL )
    return;

  switch(type){
    case WStype_CONNECTED:
      if( g_payload != NULL ){
        free(g_payload);
        g_payload = NULL;
      }
      g_type = 1;
      break;
    case WStype_DISCONNECTED:
      if( g_payload != NULL ){
        free(g_payload);
        g_payload = NULL;
      }
      g_type = 2;
      break;
    case WStype_TEXT:
      if( g_payload != NULL ){
        free(g_payload);
        g_payload = NULL;
      }
      g_payload = (char *)malloc(strlen((char*)payload) + 1);
      if( g_payload == NULL )
        return;
      strcpy(g_payload, (char*)payload);
      g_type = 3;
      break;
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

static JSValue websocket_client_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_callback_func != JS_UNDEFINED )
    JS_FreeValue(g_ctx, g_callback_func);

  g_ctx = ctx;
  g_callback_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static JSValue websocket_client_send(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !webSocketClient.isConnected() )
    return JS_EXCEPTION;

  const char* p_payload = JS_ToCString(ctx, argv[0]);
  bool ret = webSocketClient.sendTXT(p_payload);
  JS_FreeCString(ctx, p_payload);

  return JS_NewBool(ctx, ret);
}

static JSValue websocket_client_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( webSocketClient.isConnected() )
    return JS_EXCEPTION;

  const char* p_host = JS_ToCString(ctx, argv[0]);
  uint32_t port;
  JS_ToUint32(ctx, &port, argv[1]);
  const char* p_path = JS_ToCString(ctx, argv[2]);

  webSocketClient.begin(p_host, port, p_path);
  webSocketClient.onEvent(onWebsocketsEvent);
  webSocketClient.setReconnectInterval(0);

  JS_FreeCString(ctx, p_host);
  JS_FreeCString(ctx, p_path);

  return JS_UNDEFINED;
}

static JSValue websocket_client_disconnect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  webSocketClient.disconnect();
  g_type = 0;
  if( g_payload != NULL ){
    free(g_payload);
    g_payload = NULL;
  }

  return JS_UNDEFINED;
}

static JSValue websocket_client_is_connected(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, webSocketClient.isConnected());
}

static const JSCFunctionListEntry websocket_client_funcs[] = {
    JSCFunctionListEntry{"setCallback", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, websocket_client_setCallback}
                         }},
    JSCFunctionListEntry{"send", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, websocket_client_send}
                         }},
    JSCFunctionListEntry{"isConnected", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, websocket_client_is_connected}
                         }},
    JSCFunctionListEntry{"connect", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, websocket_client_connect}
                         }},
    JSCFunctionListEntry{"disconnect", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, websocket_client_disconnect}
                         }},
};

JSModuleDef *addModule_websocket_client(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "WebsocketClient", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, websocket_client_funcs,
              sizeof(websocket_client_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, websocket_client_funcs,
        sizeof(websocket_client_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_websocket_client(void)
{
  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  g_ctx = NULL;

  webSocketClient.disconnect();
  webSocketClient.begin(NULL, 0, NULL);
  g_type = 0;
  if( g_payload != NULL ){
    free(g_payload);
    g_payload = NULL;
  }
}

void loopModule_websocket_client(void){
  webSocketClient.loop();

  if( g_ctx != NULL ){
    if( g_type != 0){
        JSValue objs[2];
        if( g_type == 1 ){
          objs[0] = JS_NewString(g_ctx, "connected");
          objs[1] = JS_UNDEFINED;
        }else if( g_type == 2 ){
          objs[0] = JS_NewString(g_ctx, "disconnected");
          objs[1] = JS_UNDEFINED;
        }else if( g_type == 3 ){
          objs[0] = JS_NewString(g_ctx, "text");
          objs[1] = JS_NewString(g_ctx, g_payload);
        }else{
          return;
        }
        g_type = 0;
        if( g_payload != NULL ){
          free(g_payload);
          g_payload = NULL;
        }

        ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
        JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
        JS_FreeValue(g_ctx, objs[0]);
        JS_FreeValue(g_ctx, objs[1]);
        JS_FreeValue(g_ctx, ret);
    }
  }
}

JsModuleEntry websocket_client_module = {
  NULL,
  addModule_websocket_client,
  loopModule_websocket_client,
  endModule_websocket_client
};

#endif