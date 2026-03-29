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
  WSCB_CONNECTED,
  WSCB_DISCONNECTED,
  WSCB_TEXT,
  WSCB_BINARY,
} WSCB_TYPE;

typedef struct {
  WSCB_TYPE type;
  uint8_t *p_payload;
  uint32_t length;
} WSCLIENT_EVENT_INFO;
static std::vector<WSCLIENT_EVENT_INFO> g_event_list;

static JSContext *g_ctx = NULL;
static JSValue g_callback_func = JS_UNDEFINED;

void onMessageCallback(WebsocketsMessage message) {
  WSCLIENT_EVENT_INFO info;
  if( message.isText() ){
    info.type = WSCB_TEXT;
    info.p_payload = (uint8_t*)strdup(message.c_str());
    if( info.p_payload == NULL )
      return;
    g_event_list.push_back(info);
  }else if( message.isBinary() ){
    const char* data = message.data().c_str();
    size_t len = message.length();
    info.p_payload = (uint8_t*)malloc(len);
    if( info.p_payload == NULL )
      return;
    memmove(info.p_payload, data, len);
    info.length = len;
    g_event_list.push_back(info);
  }else{
    return;
  }
}

void onEventsCallback(WebsocketsEvent event, String data) {
    WSCLIENT_EVENT_INFO info = {};
    if(event == WebsocketsEvent::ConnectionOpened) {
      info.type = WSCB_CONNECTED;
      info.p_payload = NULL;
      g_event_list.push_back(info);
    } else if(event == WebsocketsEvent::ConnectionClosed) {
      info.type = WSCB_DISCONNECTED;
      info.p_payload = NULL;
      g_event_list.push_back(info);
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

  while(g_event_list.size() > 0){
    WSCLIENT_EVENT_INFO info = g_event_list.front();
    if( info.p_payload != NULL )
      free(info.p_payload);
    g_event_list.erase(g_event_list.begin());
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
  wsc.close();

  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  while(g_event_list.size() > 0){
    WSCLIENT_EVENT_INFO info = g_event_list.front();
    if( info.p_payload != NULL )
      free(info.p_payload);
    g_event_list.erase(g_event_list.begin());
  }

  g_ctx = NULL;
}

void loopModule_websocket_client(void)
{
  wsc.poll();

  if( g_ctx != NULL && g_callback_func != JS_UNDEFINED ){
    while(g_event_list.size() > 0){
      WSCLIENT_EVENT_INFO info = g_event_list.front();
      JSValue objs[2] = { JS_UNDEFINED, JS_UNDEFINED };
      if( info.type == WSCB_CONNECTED ){
        objs[0] = JS_NewString(g_ctx, "connected");
      }else if( info.type == WSCB_DISCONNECTED ){
        objs[0] = JS_NewString(g_ctx, "disconnected");
      }else if( info.type == WSCB_TEXT ){
        objs[0] = JS_NewString(g_ctx, "text");
        objs[1] = JS_NewString(g_ctx, (char*)info.p_payload);
        free(info.p_payload);
        info.p_payload = NULL;
      }else if( info.type == WSCB_BINARY ){
        objs[0] = JS_NewString(g_ctx, "binary");
        objs[1] = JS_NewArrayBuffer(g_ctx, info.p_payload, info.length, my_mem_free, NULL, false);
        free(info.p_payload);
        info.p_payload = NULL;
      }

      ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
      JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 2, objs);
      JS_FreeValue(g_ctx, objs[0]);
      JS_FreeValue(g_ctx, objs[1]);
      JS_FreeValue(g_ctx, ret);

      g_event_list.erase(g_event_list.begin());
    }
  }
}

JsModuleEntry websocket_client_module = {
  "WebsocketClient",
  initialize_websocket_client,
  addModule_websocket_client,
  loopModule_websocket_client,
  endModule_websocket_client
};

#endif