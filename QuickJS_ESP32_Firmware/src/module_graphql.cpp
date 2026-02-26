#include <Arduino.h>
#include "main_config.h"

#ifdef _GRAPHQL_ENABLE_

#include <HTTPClient.h>
#include "module_graphql.h"
#include "module_utils.h"
#include <string>
#include <map>

static std::map<std::string, std::string> g_headers;
static char *g_endpoint = NULL;

static String processGraphql(const char *p_body);

static JSValue graphql_setEndpoint(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *endpoint = JS_ToCString(ctx, argv[0]);
  if( endpoint == NULL )
    return JS_EXCEPTION;

  if( g_endpoint != NULL ){
    free(g_endpoint);
    g_endpoint = NULL;
  }
  g_endpoint = strdup(endpoint);
  JS_FreeCString(ctx, endpoint);
  if( g_endpoint == NULL )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue graphql_clearHeader(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  g_headers.clear();

  return JS_UNDEFINED;
}

static JSValue graphql_setHeader(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *key = JS_ToCString(ctx, argv[0]);
  if( key == NULL )
    return JS_EXCEPTION;
  const char *value = JS_ToCString(ctx, argv[1]);
  if( value == NULL ){
    JS_FreeCString(ctx, key);
    return JS_EXCEPTION;
  }

  g_headers[key] = value;
  JS_FreeCString(ctx, key);
  JS_FreeCString(ctx, value);

  return JS_UNDEFINED;
}

static JSValue graphql_execute(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( g_endpoint == NULL )
    return JS_EXCEPTION;

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "query", JS_DupValue(ctx, argv[0]));
  JS_SetPropertyStr(ctx, obj, "variables", JS_DupValue(ctx, argv[1]));
  JSValue body_object = JS_JSONStringify(ctx, obj, JS_UNDEFINED, JS_UNDEFINED);
  JS_FreeValue(ctx, obj);

  const char *body = JS_ToCString(ctx, body_object);
  if( body == NULL ){
    JS_FreeValue(ctx, body_object);
    return JS_EXCEPTION;
  }
  String result = processGraphql(body);
  JS_FreeCString(ctx, body);
  JS_FreeValue(ctx, body_object);
  if( result.length() == 0 )
    return JS_EXCEPTION;

  JSValue result_object = JS_ParseJSON(ctx, result.c_str(), result.length(), "json");

  return result_object;
}

static const JSCFunctionListEntry graphql_funcs[] = {
    JSCFunctionListEntry{
        "setEndpoint", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, graphql_setEndpoint}
        }},
    JSCFunctionListEntry{
        "setHeader", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, graphql_setHeader}
        }},
    JSCFunctionListEntry{
        "clearHeader", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, graphql_clearHeader}
        }},
    JSCFunctionListEntry{
        "query", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, graphql_execute}
        }},
    JSCFunctionListEntry{
        "mutation", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, graphql_execute}
        }},
};

JSModuleDef *addModule_graphql(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Graphql", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, graphql_funcs,
                            sizeof(graphql_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, graphql_funcs,
        sizeof(graphql_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_graphql(void){
  g_headers.clear();
  if( g_endpoint != NULL ){
    free(g_endpoint);
    g_endpoint = NULL;
  }
}

JsModuleEntry graphql_module = {
  NULL,
  addModule_graphql,
  NULL,
  endModule_graphql
};

static String processGraphql(const char *p_body)
{
  HTTPClient http;
  http.begin(g_endpoint);

  http.addHeader("Content-Type", "application/json");
  for (const auto& kv : g_headers) {
    const std::string& key = kv.first;
    const std::string& value = kv.second;
    http.addHeader(key.c_str(), value.c_str());
  }

  int httpCode = http.POST(p_body);
  if ((httpCode >= 200 && httpCode < 300) || httpCode == 400) {
    String payload = http.getString();
    http.end();
    return payload;
  }else{
    http.end();
    return String("");
  }
}

#endif
