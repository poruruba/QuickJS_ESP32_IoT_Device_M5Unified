#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "quickjs.h"
#include "main_config.h"
#include "module_type.h"
#include "module_utils.h"

#define HTTP_RESP_SHIFT   0
#define HTTP_RESP_NONE    0x0
#define HTTP_RESP_TEXT    0x1
#define HTTP_RESP_JSON    0x2
#define HTTP_RESP_BINARY  0x3
#define HTTP_RESP_MASK   0x07

#define HTTP_METHOD_SHIFT          8
#define HTTP_METHOD_GET            0x0
#define HTTP_METHOD_POST_JSON      0x1
#define HTTP_METHOD_POST_URLENCODE 0x2
#define HTTP_METHOD_POST_FORMDATA  0x3
#define HTTP_METHOD_MASK           0x07

static JSValue http_bridge(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  uint16_t method = (magic >> HTTP_METHOD_SHIFT) & HTTP_METHOD_MASK;
  const char *p_target_type;
  switch(method){
    case HTTP_METHOD_GET: p_target_type = "get"; break;
    case HTTP_METHOD_POST_JSON: p_target_type = "post_json"; break;
    case HTTP_METHOD_POST_FORMDATA: p_target_type = "post_form-data"; break;
    case HTTP_METHOD_POST_URLENCODE: p_target_type = "post_x-www-form-urlencoded"; break;
    default: {
      return JS_EXCEPTION;
    }
  }

  const char *target_host = JS_ToCString(ctx, argv[0]);
  if( target_host == NULL ){
    return JS_EXCEPTION;
  }

  JSValue obj = JS_NewObject(ctx);
  if (argc >= 1)
    JS_SetPropertyStr(ctx, obj, "qs", JS_DupValue(ctx, argv[1]));
  if (argc >= 2){
    if( method == HTTP_METHOD_POST_JSON)
      JS_SetPropertyStr(ctx, obj, "body", JS_DupValue(ctx, argv[2]));
    else if( method != HTTP_METHOD_GET)
      JS_SetPropertyStr(ctx, obj, "params", JS_DupValue(ctx, argv[2]));
  }
  if (argc >= 3)
    JS_SetPropertyStr(ctx, obj, "headers", JS_DupValue(ctx, argv[3]));

  JSValue json = JS_JSONStringify(ctx, obj, JS_UNDEFINED, JS_UNDEFINED);
  JS_FreeValue(ctx, obj);
  if( json == JS_UNDEFINED ){
    JS_FreeCString(ctx, target_host);
    return JS_EXCEPTION;
  }
  const char *body = JS_ToCString(ctx, json);
  if (body == NULL){
    JS_FreeCString(ctx, target_host);
    JS_FreeValue(ctx, json);
    return JS_EXCEPTION;
  }

  String server = read_config_string(CONFIG_FNAME_BRIDGE);

  Serial.printf("target_host=%s\n", target_host);
  Serial.printf("target_type=%s\n", p_target_type);
  Serial.printf("server=%s\n", server.c_str());
  Serial.printf("body=%s\n", body);

  HTTPClient http;
  http.begin(server + "/agent"); //HTTP
  http.addHeader("Content-Type", "application/json");
  http.addHeader("target_host", target_host);
  http.addHeader("target_type", p_target_type);

  int status_code;
  // HTTP POST JSON
  status_code = http.POST(body);
  JS_FreeCString(ctx, target_host);
  JS_FreeCString(ctx, body);
  JS_FreeValue(ctx, json);

  uint8_t response_type = ( magic >> HTTP_RESP_SHIFT ) & HTTP_RESP_MASK;
  JSValue value = JS_EXCEPTION;
  if (status_code != 200){
    Serial.printf("status_code=%d\n", status_code);
    goto end;
  }

  if (response_type == HTTP_RESP_JSON ){
    String result = http.getString();
    const char *buffer = result.c_str();
    value = JS_ParseJSON(ctx, buffer, strlen(buffer), "json");
  }else if( response_type == HTTP_RESP_TEXT ){
    String result = http.getString();
    const char *buffer = result.c_str();
    value = JS_NewString(ctx, buffer);
  }else if( response_type == HTTP_RESP_BINARY ){
    String result = http.getString();
    const char *b64 = result.c_str();
    unsigned long binlen = b64_decode_length(b64);
    unsigned char *bin = (unsigned char*)malloc(binlen);
    if( bin == NULL )
      goto end;
    b64_decode(b64, bin);

    value = JS_NewArrayBufferCopy(ctx, bin, binlen);
    free(bin);
  }else if( response_type == HTTP_RESP_NONE ){
    value = JS_UNDEFINED;
  }

end:
  http.end();

  return value;
}

static JSValue http_fetch(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( argc <= 1 )
    return JS_EXCEPTION;

  int32_t type;
  JS_ToInt32(ctx, &type, argv[0]);

  return http_bridge(ctx, jsThis, argc - 1, &argv[1], type);
}

static JSValue http_setHttpBridgeServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *url = JS_ToCString(ctx, argv[0]);
  if( url == NULL )
    return JS_EXCEPTION;

  long ret;  
  ret = write_config_string(CONFIG_FNAME_BRIDGE, url);
  JS_FreeCString(ctx, url);
  if( ret != 0 )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue http_getHttpBridgeServer(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  String url = read_config_string(CONFIG_FNAME_BRIDGE);

  return JS_NewString(ctx, url.c_str());
}

static const JSCFunctionListEntry http_funcs[] = {
    JSCFunctionListEntry{"fetch", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic, http_fetch}
                         }},
    JSCFunctionListEntry{"setHttpBridgeServer", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, http_setHttpBridgeServer}
                         }},
    JSCFunctionListEntry{"getHttpBridgeServer", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, http_getHttpBridgeServer}
                         }},
    JSCFunctionListEntry{
        "resp_none", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_RESP_NONE << HTTP_RESP_SHIFT)
        }},
    JSCFunctionListEntry{
        "resp_text", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_RESP_TEXT << HTTP_RESP_SHIFT)
        }},
    JSCFunctionListEntry{
        "resp_json", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_RESP_JSON << HTTP_RESP_SHIFT)
        }},
    JSCFunctionListEntry{
        "resp_binary", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_RESP_BINARY << HTTP_RESP_SHIFT)
        }},
    JSCFunctionListEntry{
        "method_get", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_METHOD_GET << HTTP_METHOD_SHIFT)
        }},
    JSCFunctionListEntry{
        "method_post_json", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_METHOD_POST_JSON << HTTP_METHOD_SHIFT)
        }},
    JSCFunctionListEntry{
        "method_post_urlencode", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_METHOD_POST_URLENCODE << HTTP_METHOD_SHIFT)
        }},
    JSCFunctionListEntry{
        "method_post_formdata", 0, JS_DEF_PROP_INT32, 0, {
          i32 : (HTTP_METHOD_POST_FORMDATA << HTTP_METHOD_SHIFT)
        }},
};

JSModuleDef *addModule_http(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Http", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, http_funcs,
                            sizeof(http_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, http_funcs,
        sizeof(http_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry http_module = {
  NULL,
  addModule_http,
  NULL,
  NULL
};