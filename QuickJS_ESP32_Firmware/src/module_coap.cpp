#include <Arduino.h>
#include "main_config.h"

#ifdef _COAP_ENABLE_

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_coap.h"
#include <IPAddress.h>

static WiFiUDP udp;
static Coap coap(udp);

#define MAX_COAP_EVENT  4

static JSContext *g_ctx;
static JSValue g_callback_func = JS_UNDEFINED;
static uint16_t g_token = 0;

typedef struct{
  CoapPacket packet;
  uint32_t remote_ip;
  int remote_port;
} COAP_EVENT_INFO;
static std::vector<COAP_EVENT_INFO> g_event_list;

static uint8_t simple_code_to_rfc(uint8_t simple_code)
{
    uint8_t cls = simple_code / 10;
    uint8_t det = simple_code % 10;
    cls = cls % 10;

    return (cls << 5) | det;
}

static void coap_callback_response(CoapPacket &packet, IPAddress ip, int port)
{
//  Serial.printf("CoAP packet received from %d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3], port);
  if( g_event_list.size() >= MAX_COAP_EVENT )
    return;

  COAP_EVENT_INFO info;
  info.packet = packet;
  info.remote_ip = (((uint32_t)ip[0]) << 24) | (((uint32_t)ip[1]) << 16) | (((uint32_t)ip[2]) << 8) | ip[3];
  info.remote_port = port;

  g_event_list.push_back(info);
}

static JSValue coap_get_delete(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  const char *ipaddress = JS_ToCString(ctx, argv[0]);
  if( ipaddress == NULL )
    return JS_EXCEPTION;
  uint32_t port;
  JS_ToUint32(ctx, &port, argv[1]);
  const char *url = JS_ToCString(ctx, argv[2]);
  if( url == NULL ){
    JS_FreeCString(ctx, ipaddress);
    return JS_EXCEPTION;
  }
  IPAddress ip;
  ip.fromString(ipaddress);
  JS_FreeCString(ctx, ipaddress);

  g_token++;
  uint8_t tmp[2] = { (uint8_t)((g_token >> 8) & 0xff), (uint8_t)(g_token & 0xff) };
  uint16_t ret = coap.send(
      ip, port, url,
      COAP_CON, magic == 1 ? COAP_DELETE : COAP_GET,
      tmp, 2,
      NULL, 0
  );
  JS_FreeCString(ctx, url);
  if( ret == 0 )
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, ret);
}

static JSValue coap_post_put(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  const char *ipaddress = JS_ToCString(ctx, argv[0]);
  if( ipaddress == NULL )
    return JS_EXCEPTION;
  uint32_t port;
  JS_ToUint32(ctx, &port, argv[1]);
  const char *url = JS_ToCString(ctx, argv[2]);
  if( url == NULL ){
    JS_FreeCString(ctx, ipaddress);
    return JS_EXCEPTION;
  }
  IPAddress ip;
  ip.fromString(ipaddress);
  JS_FreeCString(ctx, ipaddress);

  bool is_object = false;
  const char *payload = NULL;
  if( JS_IsObject(argv[3]) ){
    is_object = true;
    JSValue value = JS_JSONStringify(ctx, argv[3], JS_UNDEFINED, JS_UNDEFINED);
    payload = JS_ToCString(ctx, value);
    JS_FreeValue(ctx, value);
  }else if( JS_IsString(argv[3]) ){
    is_object = false;
    payload = JS_ToCString(ctx, argv[3]);
  }else{
    JS_FreeCString(ctx, url);
    return JS_EXCEPTION;
  }
  if( payload == NULL ){
    JS_FreeCString(ctx, url);
    return JS_EXCEPTION;
  }

  g_token++;
  uint8_t tmp[2] = { (uint8_t)((g_token >> 8) & 0xff), (uint8_t)(g_token & 0xff) };
  uint16_t ret = coap.send(
      ip, port, url,
      COAP_CON,
      magic == 1 ? COAP_PUT : COAP_POST,
      tmp, 2,
      (uint8_t*)payload, strlen(payload),
      is_object ? COAP_APPLICATION_JSON : COAP_TEXT_PLAIN
  );
  JS_FreeCString(ctx, payload);
  JS_FreeCString(ctx, url);
  if( ret == 0 )
    return JS_EXCEPTION;

  return JS_NewInt32(ctx, ret);
}

static JSValue coap_setCallback(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_ctx = ctx;

  if(g_callback_func != JS_UNDEFINED){
    JS_FreeValue(ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  g_callback_func = JS_DupValue(ctx, argv[0]);

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry coap_funcs[] = {
    JSCFunctionListEntry{
        "get", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic_magic, {generic_magic : coap_get_delete}}
        }},
    JSCFunctionListEntry{
        "delete", 0, JS_DEF_CFUNC, 1, {
          func : {3, JS_CFUNC_generic_magic, {generic_magic : coap_get_delete}}
        }},
    JSCFunctionListEntry{
        "post", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic_magic, {generic_magic : coap_post_put}}
        }},
    JSCFunctionListEntry{
        "put", 0, JS_DEF_CFUNC, 1, {
          func : {4, JS_CFUNC_generic_magic, {generic_magic : coap_post_put}}
        }},
    JSCFunctionListEntry{
        "setCallback", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, coap_setCallback}
        }},
};

JSModuleDef *addModule_coap(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Coap", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, coap_funcs,
                            sizeof(coap_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, coap_funcs,
        sizeof(coap_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void loopModule_coap(void){
  coap.loop();

  if( g_ctx != NULL && g_callback_func != JS_UNDEFINED ){
    while(g_event_list.size() > 0){
      COAP_EVENT_INFO info = g_event_list.front();

      JSValue obj = JS_NewObject(g_ctx);

      JS_SetPropertyStr(g_ctx, obj, "type", JS_NewUint32(g_ctx, info.packet.type));
      JS_SetPropertyStr(g_ctx, obj, "code", JS_NewUint32(g_ctx, simple_code_to_rfc(info.packet.code)));
      JS_SetPropertyStr(g_ctx, obj, "messageid", JS_NewUint32(g_ctx, info.packet.messageid));

      if( info.packet.payloadlen > 0 ){
        String payload = String((const char*)info.packet.payload, info.packet.payloadlen);
        JS_SetPropertyStr(g_ctx, obj, "payload", JS_NewString(g_ctx, payload.c_str()));
      }
      
      JS_SetPropertyStr(g_ctx, obj, "remote_ip", JS_NewUint32(g_ctx, info.remote_ip));
      JS_SetPropertyStr(g_ctx, obj, "remote_port", JS_NewInt32(g_ctx, info.remote_port));

      ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(g_ctx);
      JSValue ret = qjs->callJsFunc_with_arg(g_ctx, g_callback_func, g_callback_func, 1, &obj);
      JS_FreeValue(g_ctx, obj);
      JS_FreeValue(g_ctx, ret);

      g_event_list.erase(g_event_list.begin());
    }
  }
}

long initialize_coap(void){
  coap.response(coap_callback_response);
  coap.start();

  return 0;
}

void endModule_coap(void){
  if( g_callback_func != JS_UNDEFINED ){
    JS_FreeValue(g_ctx, g_callback_func);
    g_callback_func = JS_UNDEFINED;
  }

  while(g_event_list.size() > 0){
    COAP_EVENT_INFO info = g_event_list.front();
    g_event_list.erase(g_event_list.begin());
  }
}

JsModuleEntry coap_module = {
  initialize_coap,
  addModule_coap,
  loopModule_coap,
  endModule_coap
};

#endif
