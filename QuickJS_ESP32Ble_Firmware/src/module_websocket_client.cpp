#include <Arduino.h>
#include "main_config.h"

#ifdef _WEBSOCKET_CLIENT_ENABLE_

#include "quickjs.h"
#include "module_type.h"
#include "module_websocket_client.h"
#include "quickjs_esp32.h"
#include <ArduinoWebsockets.h>

using namespace websockets;
static WebsocketsClient wsc;

typedef enum {
  WSCB_NONE,
  WSCB_CONNECTED,
  WSCB_DISCONNECTED,
  WSCB_TEXT
} WSCB_TYPE;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;
static WSCB_TYPE g_type = WSCB_NONE;
static char *g_payload = NULL;

void onMessageCallback(WebsocketsMessage message) {
  if( g_payload != NULL ){
    free(g_payload);
    g_payload = NULL;
  }
  if( !message.isText() )
    return;

  const char *payload = message.c_str();
  g_payload = (char *)malloc(strlen(payload) + 1);
  if( g_payload == NULL )
    return;
  strcpy(g_payload, (char*)payload);
  g_type = WSCB_TEXT;
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
      if( g_payload != NULL ){
        free(g_payload);
        g_payload = NULL;
      }
      g_type = WSCB_CONNECTED;
    } else if(event == WebsocketsEvent::ConnectionClosed) {
      if( g_payload != NULL ){
        free(g_payload);
        g_payload = NULL;
      }
      g_type = WSCB_DISCONNECTED;
    } else if(event == WebsocketsEvent::GotPing) {
//        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
//        Serial.println("Got a Pong!");
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
  if( !wsc.available() )
    return JS_EXCEPTION;

  const char* p_payload = JS_ToCString(ctx, argv[0]);
  bool ret = wsc.send(p_payload);
  JS_FreeCString(ctx, p_payload);

  return JS_NewBool(ctx, ret);
}

static JSValue websocket_client_connect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( wsc.available() )
    return JS_EXCEPTION;

  const char* p_host = JS_ToCString(ctx, argv[0]);
  uint32_t port;
  JS_ToUint32(ctx, &port, argv[1]);
  const char* p_path = JS_ToCString(ctx, argv[2]);

  bool ret = wsc.connect(p_host, port, p_path);

  JS_FreeCString(ctx, p_host);
  JS_FreeCString(ctx, p_path);

  return JS_NewBool(ctx, ret);
}

static JSValue websocket_client_disconnect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  wsc.close();

  g_type = WSCB_NONE;
  if( g_payload != NULL ){
    free(g_payload);
    g_payload = NULL;
  }

  return JS_UNDEFINED;
}

static JSValue websocket_client_is_connected(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, wsc.available());
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

long initialize_websocket_client(void)
{
  wsc.onMessage(onMessageCallback);
  wsc.onEvent(onEventsCallback);
  
  return 0;
}

void endModule_websocket_client(void)
{
  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }
  g_ctx = NULL;

  wsc.close();

  g_type = WSCB_NONE;
  if( g_payload != NULL ){
    free(g_payload);
    g_payload = NULL;
  }
}

void loopModule_websocket_client(void)
{
  wsc.poll();

  if( g_ctx != NULL ){
    if( g_type != WSCB_NONE){
      JSValue objs[2];
      if( g_type == WSCB_CONNECTED ){
        objs[0] = JS_NewString(g_ctx, "connected");
        objs[1] = JS_UNDEFINED;
      }else if( g_type == WSCB_DISCONNECTED ){
        objs[0] = JS_NewString(g_ctx, "disconnected");
        objs[1] = JS_UNDEFINED;
      }else if( g_type == WSCB_TEXT ){
        objs[0] = JS_NewString(g_ctx, "text");
        objs[1] = JS_NewString(g_ctx, g_payload);
      }else{
        return;
      }
      g_type = WSCB_NONE;
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
  initialize_websocket_client,
  addModule_websocket_client,
  loopModule_websocket_client,
  endModule_websocket_client
};

#endif