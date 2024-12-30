#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "quickjs.h"
#include "main_config.h"
#include "module_type.h"
#include "module_utils.h"
#include "config_utils.h"
#include <mbedtls/md.h>

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

static const char *aws4_request = "aws4_request";
static const char *signedHeaderNamesBase = "host;x-amz-content-sha256;x-amz-date";
static const char *default_region = "ap-northeast-1";

typedef struct _AwsAuthorizationResult {
  char amzDate[16 + 1];
  char payloadHash[32 * 2 + 1];
  char *authorization;
  long result;
} AwsAuthorizationResult;

static AwsAuthorizationResult makeAwsAuthorization(const char *method, const char *host, const char *canonicalUri, const char *canonicalQuerystring,
                            const char *canonicalHeaders, const char *canonicalHeaderNames, const unsigned char *payload, int payload_length, 
                            const char *service, const char *region, const char *accessKeyId, const char *secretAccessKey);

static JSValue aws_bridge(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  String accessKeyId = read_config_string(CONFIG_AWS_CREDENTIAL1);
  String secretAccessKey = read_config_string(CONFIG_AWS_CREDENTIAL2);
  String sessionToken = read_config_string(CONFIG_AWS_CREDENTIAL3);

  JSValue method = JS_GetPropertyStr(ctx, argv[0], "method");
  if( method == JS_UNDEFINED )
    return JS_EXCEPTION;
  const char *method_str = JS_ToCString(ctx, method);
  JS_FreeValue(ctx, method);
  JSValue host = JS_GetPropertyStr(ctx, argv[0], "host");
  if( host == JS_UNDEFINED )
    return JS_EXCEPTION;
  const char *host_str = JS_ToCString(ctx, host);
  JS_FreeValue(ctx, host);
  JSValue canonicalUri = JS_GetPropertyStr(ctx, argv[0], "canonicalUri");
  if( canonicalUri == JS_UNDEFINED )
    return JS_EXCEPTION;
  const char *canonicalUri_str = JS_ToCString(ctx, canonicalUri);
  JS_FreeValue(ctx, canonicalUri);
  JSValue canonicalQuerystring = JS_GetPropertyStr(ctx, argv[0], "canonicalQuerystring");
  const char *canonicalQuerystring_str = NULL;
  if( canonicalQuerystring != JS_UNDEFINED ){
    canonicalQuerystring_str = JS_ToCString(ctx, canonicalQuerystring);
    JS_FreeValue(ctx, canonicalQuerystring);
  }
  JSValue canonicalHeaders = JS_GetPropertyStr(ctx, argv[0], "canonicalHeaders");
  const char *canonicalHeaders_str = NULL;
  if( canonicalHeaders != JS_UNDEFINED ){
    canonicalHeaders_str = JS_ToCString(ctx, canonicalHeaders);
    JS_FreeValue(ctx, canonicalHeaders);
  }
  JSValue canonicalHeaderNames = JS_GetPropertyStr(ctx, argv[0], "canonicalHeaderNames");
  const char *canonicalHeaderNames_str = NULL;
  if( canonicalHeaderNames != JS_UNDEFINED ){
    canonicalHeaderNames_str = JS_ToCString(ctx, canonicalHeaderNames);
    JS_FreeValue(ctx, canonicalHeaderNames);
  }
  JSValue service = JS_GetPropertyStr(ctx, argv[0], "service");
  if( service == JS_UNDEFINED )
    return JS_EXCEPTION;
  const char *service_str = JS_ToCString(ctx, service);
  JS_FreeValue(ctx, service);
  JSValue contentType = JS_GetPropertyStr(ctx, argv[0], "contentType");
  const char *contentType_str = NULL;
  if( contentType != JS_UNDEFINED ){
    contentType_str = JS_ToCString(ctx, contentType);
    JS_FreeValue(ctx, contentType);
  }
  JSValue region = JS_GetPropertyStr(ctx, argv[0], "region");
  const char *region_str = default_region;
  if( region != JS_UNDEFINED ){
    region_str = JS_ToCString(ctx, region);
    JS_FreeValue(ctx, region);
  }

  JSValue payload = JS_GetPropertyStr(ctx, argv[0], "payload");
  const char *payload_str = NULL;
  if( payload != JS_UNDEFINED ){
    payload_str = JS_ToCString(ctx, payload);
    JS_FreeValue(ctx, payload);
  }

  AwsAuthorizationResult amzResult = makeAwsAuthorization(method_str, host_str, canonicalUri_str, canonicalQuerystring_str, canonicalHeaders_str, canonicalHeaderNames_str, (uint8_t*)payload_str, strlen(payload_str), service_str, region_str, accessKeyId.c_str(), secretAccessKey.c_str());
  if( region_str != default_region )
    JS_FreeCString(ctx, region_str);
  JS_FreeCString(ctx, service_str);
  if( amzResult.result != 0 ){
    JS_FreeCString(ctx, canonicalUri_str);
    if( canonicalQuerystring_str != NULL )
      JS_FreeCString(ctx, canonicalQuerystring_str);
    JS_FreeCString(ctx, host_str);
    if( canonicalHeaders_str != NULL )
      JS_FreeCString(ctx, canonicalHeaders_str);
    if( payload_str != NULL )
      JS_FreeCString(ctx, payload_str);
    if( contentType_str != NULL )
      JS_FreeCString(ctx, contentType_str);

    return JS_EXCEPTION;
  }

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "method", JS_NewString(ctx, method_str));
  JS_FreeCString(ctx, method_str);
  JS_SetPropertyStr(ctx, obj, "canonicalUri", JS_NewString(ctx, canonicalUri_str));
  JS_FreeCString(ctx, canonicalUri_str);
  if( canonicalQuerystring_str != NULL ){
    JS_SetPropertyStr(ctx, obj, "canonicalQuerystring", JS_NewString(ctx, canonicalQuerystring_str));
    JS_FreeCString(ctx, canonicalQuerystring_str);
  }
  JS_SetPropertyStr(ctx, obj, "host", JS_NewString(ctx, host_str));
  String headers = String("host:") + host_str + "\nx-amz-content-sha256:" + amzResult.payloadHash + "\nx-amz-date:" + amzResult.amzDate + "\n";
  JS_FreeCString(ctx, host_str);
  if( canonicalHeaders_str != NULL ){
    headers += canonicalHeaders_str;
    JS_FreeCString(ctx, canonicalHeaders_str);
  }
  if( sessionToken != NULL && sessionToken.length() > 0 )
    headers += String("x-amz-security-token:") + sessionToken + "\n";
  headers += String("authorization:") + amzResult.authorization + "\n";

  free(amzResult.authorization);
  JS_SetPropertyStr(ctx, obj, "headers", JS_NewString(ctx, headers.c_str()));
  if( payload_str != NULL ){
    JS_SetPropertyStr(ctx, obj, "payload", JS_NewString(ctx, payload_str));
    JS_FreeCString(ctx, payload_str);
  }
  if( contentType_str != NULL ){
    JS_SetPropertyStr(ctx, obj, "content_type", JS_NewString(ctx, contentType_str));
    JS_FreeCString(ctx, contentType_str);
  }

  JSValue json = JS_JSONStringify(ctx, obj, JS_UNDEFINED, JS_UNDEFINED);
  JS_FreeValue(ctx, obj);
  if( json == JS_UNDEFINED )
    return JS_EXCEPTION;
  const char *body = JS_ToCString(ctx, json);
  JS_FreeValue(ctx, json);
  if (body == NULL)
    return JS_EXCEPTION;

  String server = read_config_string(CONFIG_FNAME_BRIDGE);

  bool sem = xSemaphoreTake(binSem, portMAX_DELAY);
  HTTPClient http;
  http.begin(server + "/aws"); //HTTP
  http.addHeader("Content-Type", "application/json");

  // HTTP POST JSON
  int status_code = http.POST(body);
  JS_FreeCString(ctx, body);

  JSValue value = JS_EXCEPTION;
  if (status_code != 200){
    Serial.printf("status_code=%d\n", status_code);
  }else{
    String result = http.getString();
    value = JS_NewString(ctx, result.c_str());
  }

end:
  http.end();
  if( sem )
    xSemaphoreGive(binSem);
  return value;
}

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
  JS_FreeValue(ctx, json);
  if (body == NULL){
    JS_FreeCString(ctx, target_host);
    return JS_EXCEPTION;
  }

  String server = read_config_string(CONFIG_FNAME_BRIDGE);

  // Serial.printf("target_host=%s\n", target_host);
  // Serial.printf("target_type=%s\n", p_target_type);
  // Serial.printf("server=%s\n", server.c_str());
  // Serial.printf("body=%s\n", body);

  bool sem = xSemaphoreTake(binSem, portMAX_DELAY);
  HTTPClient http;
  http.begin(server + "/agent"); //HTTP
  http.addHeader("Content-Type", "application/json");
  http.addHeader("target_host", target_host);
  http.addHeader("target_type", p_target_type);
  JS_FreeCString(ctx, target_host);

  // HTTP POST JSON
  int status_code = http.POST(body);
  JS_FreeCString(ctx, body);

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
  if( sem )
    xSemaphoreGive(binSem);

  return value;
}

static JSValue http_setAwsCredential(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *accessKeyId = JS_ToCString(ctx, argv[0]);
  if( accessKeyId == NULL )
    return JS_EXCEPTION;
  const char *secretAccessKey = JS_ToCString(ctx, argv[1]);
  if( secretAccessKey == NULL )
    return JS_EXCEPTION;
  const char *sessionToken = NULL;
  if( argc > 2 ){
    sessionToken = JS_ToCString(ctx, argv[2]);
    if( sessionToken != NULL && strlen(sessionToken) == 0 ){
      JS_FreeCString(ctx, sessionToken);
      sessionToken = NULL;
    }
  }

  long ret;  
  ret = write_config_string(CONFIG_AWS_CREDENTIAL1, accessKeyId);
  JS_FreeCString(ctx, accessKeyId);
  if( ret != 0 )
    return JS_EXCEPTION;
  ret = write_config_string(CONFIG_AWS_CREDENTIAL2, secretAccessKey);
  JS_FreeCString(ctx, secretAccessKey);
  if( ret != 0 )
    return JS_EXCEPTION;
  if( sessionToken == NULL ){
    ret = write_config_string(CONFIG_AWS_CREDENTIAL3, "");
  }else{
    ret = write_config_string(CONFIG_AWS_CREDENTIAL3, sessionToken);
    JS_FreeCString(ctx, sessionToken);
  }
  if( ret != 0 )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
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
    JSCFunctionListEntry{"fetchAws", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, aws_bridge}
                         }},
    JSCFunctionListEntry{"setAwsCredential", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, http_setAwsCredential}
                         }},
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


static long hmacCreate(const uint8_t *p_key, int key_len, const uint8_t *p_input, int input_len, uint8_t *p_result)
{
  mbedtls_md_context_t context;
  
  mbedtls_md_init(&context);
  mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&context, p_key, key_len);
  if( p_input != NULL )
    mbedtls_md_hmac_update(&context, p_input, input_len);
  mbedtls_md_hmac_finish(&context, p_result); // 32 bytes
  mbedtls_md_free(&context);

  return 0;
}

static long hashCreate(const uint8_t *p_input, int length, uint8_t *p_result)
{  
  mbedtls_md_context_t context;

  mbedtls_md_init(&context);
  mbedtls_md_setup(&context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  if( p_input != NULL )
    mbedtls_md_update(&context, (const unsigned char*)p_input, length);
  mbedtls_md_finish(&context, p_result); // 32 bytes

  return 0;
}

static char tohex(int i){
  if( i < 10 )
    return '0' + i;
  else if( i < 16 )
    return 'a' + (i - 10);
  else
    return '0';
}
static void toHexStr(int len, const uint8_t *p_bin, char *p_hex){
  for( int i = 0 ; i < len ; i++ ){
    p_hex[i * 2] = tohex((p_bin[i] >> 4 ) & 0x0f);
    p_hex[i * 2 + 1] = tohex(p_bin[i] & 0x0f);
  }
  p_hex[len * 2] = '\0';
}

static void getSignatureKey(const char *key, const char *dateStamp, const char *regionName, const char *serviceName, uint8_t *kSigning){
  int key_len = strlen("AWS4") + strlen(key);
  char *temp_key = (char*)malloc(key_len + 1);
  sprintf(temp_key, "%s%s", "AWS4", key);

  uint8_t kDate[32];
  hmacCreate((const uint8_t*)temp_key, strlen(temp_key), (const uint8_t*)dateStamp, strlen(dateStamp), kDate);
  free(temp_key);

  uint8_t kRegion[32];
  hmacCreate(kDate, sizeof(kDate), (const uint8_t*)regionName, strlen(regionName), kRegion);

  uint8_t kService[32];
  hmacCreate(kRegion, sizeof(kRegion), (const uint8_t*)serviceName, strlen(serviceName), kService);

  hmacCreate(kService, sizeof(kService), (const uint8_t*)aws4_request, strlen(aws4_request), kSigning);
}

static AwsAuthorizationResult makeAwsAuthorization(const char *method, const char *host, const char *canonicalUri, const char *canonicalQuerystring,
                            const char *canonicalHeaders,const char *canonicalHeaderNames, const unsigned char *payload, int payload_length, 
                            const char *service, const char *region, const char *accessKeyId, const char *secretAccessKey)
{
  // Serial.printf("method=%s\n", method);
  // Serial.printf("host=%s\n", host);
  // Serial.printf("canonicalUri=%s\n", canonicalUri);
  // if( canonicalQuerystring != NULL )
  //   Serial.printf("canonicalQuerystring=%s\n", canonicalQuerystring);
  // if( canonicalHeaders != NULL )
  //   Serial.printf("canonicalHeaders=%s\n", canonicalHeaders);
  // if( canonicalHeaderNames != NULL )
  //   Serial.printf("canonicalHeaderNames=%s\n", canonicalHeaderNames);
  // Serial.printf("service=%s\n", service);
  // if( payload != NULL )
  //   Serial.printf("payload=%s\n", payload);
  // Serial.printf("region=%s\n", region);
  // Serial.printf("accessKeyId=%s\n", accessKeyId);
  // Serial.printf("secretAccessKey=%s\n", secretAccessKey);
  
  AwsAuthorizationResult amzResult;
  amzResult.result = -1;
  amzResult.authorization = NULL;
  
  time_t now = time(nullptr);
  struct tm* utcTime = gmtime(&now); 
  if( utcTime->tm_year == 70 )
    return amzResult;

  char dateStamp[9] = "19000101";
  sprintf(dateStamp, "%04d%02d%02d", 1900 + utcTime->tm_year, utcTime->tm_mon + 1, utcTime->tm_mday);
  sprintf(amzResult.amzDate, "%04d%02d%02dT%02d%02d%02dZ", 1900 + utcTime->tm_year, utcTime->tm_mon + 1, utcTime->tm_mday, utcTime->tm_hour, utcTime->tm_min, utcTime->tm_sec);

  uint8_t payloadHash[32];
  hashCreate(payload, payload_length, payloadHash);
  toHexStr(sizeof(payloadHash), payloadHash, amzResult.payloadHash);

  String signedHeaderNames = String(signedHeaderNamesBase);
  if( canonicalHeaderNames != NULL && strlen(canonicalHeaderNames) > 0)
    signedHeaderNames += String(";") + canonicalHeaderNames;
  String headers = String("host:") + host + "\n" + "x-amz-content-sha256:" + amzResult.payloadHash + "\n" + "x-amz-date:" + amzResult.amzDate + "\n";
  if( canonicalHeaders != NULL )
     headers += String(canonicalHeaders);

  int canonicalRequest_len = strlen(method) + 1 + strlen(canonicalUri) + 1 + (canonicalQuerystring != NULL ? strlen(canonicalQuerystring) : 0) + 1 + headers.length() + 1 + signedHeaderNames.length() + 1 + strlen(amzResult.payloadHash);
  char *canonicalRequest = (char*)malloc(canonicalRequest_len + 1);
  sprintf(canonicalRequest, "%s\n%s\n%s\n%s\n%s\n%s", method, canonicalUri, (canonicalQuerystring != NULL ? canonicalQuerystring : ""), headers.c_str(), signedHeaderNames.c_str(), amzResult.payloadHash);

  uint8_t hashCanonicalRequest[32];
  hashCreate((uint8_t*)canonicalRequest, strlen(canonicalRequest), hashCanonicalRequest);
  free(canonicalRequest);
  char hashCanonicalRequest_Hex[sizeof(hashCanonicalRequest) * 2 + 1];
  toHexStr(sizeof(hashCanonicalRequest), hashCanonicalRequest, hashCanonicalRequest_Hex);

  int scope_len = strlen(dateStamp) + 1 + strlen(region) + 1 + strlen(service) + 1 + strlen(aws4_request);
  char *credentialScope = (char*)malloc(scope_len + 1);
  sprintf(credentialScope, "%s/%s/%s/%s", dateStamp, region, service, aws4_request);

  const char *algorithm = "AWS4-HMAC-SHA256";
  int stringToSign_len = strlen(algorithm) + 1 + strlen(amzResult.amzDate) + 1 + strlen(credentialScope) + 1 + strlen(hashCanonicalRequest_Hex);
  char *stringToSign = (char*)malloc(stringToSign_len + 1);
  sprintf(stringToSign, "%s\n%s\n%s\n%s", algorithm, amzResult.amzDate, credentialScope, hashCanonicalRequest_Hex);

  uint8_t signingKey[32];
  getSignatureKey(secretAccessKey, dateStamp, region, service, signingKey);
  char signingKey_Hex[sizeof(signingKey) * 2 + 1];
  toHexStr(sizeof(signingKey), signingKey, signingKey_Hex);

  uint8_t signature[32];
  hmacCreate(signingKey, sizeof(signingKey), (const uint8_t*)stringToSign, strlen(stringToSign), signature);
  free(stringToSign);
  char signature_Hex[sizeof(signature) * 2 + 1];
  toHexStr(sizeof(signature), signature, signature_Hex);

  int authorization_len = strlen(algorithm) + strlen(" Credential=") + strlen(accessKeyId) + 1 + strlen(credentialScope) + strlen(", SignedHeaders=") + signedHeaderNames.length() + strlen(", Signature=") + strlen(signature_Hex);
  char *authorization = (char*)malloc(authorization_len + 1);
  sprintf(authorization, "%s Credential=%s/%s, SignedHeaders=%s, Signature=%s", algorithm, accessKeyId, credentialScope, signedHeaderNames.c_str(), signature_Hex);
  free(credentialScope);

  amzResult.authorization = authorization;
  amzResult.result = 0;

  // Serial.printf("amzDate=%s\n", amzResult.amzDate);
  // Serial.printf("payloadHash=%s\n", amzResult.payloadHash);
  // Serial.printf("authorization=%s\n", amzResult.authorization);

  return amzResult;
}