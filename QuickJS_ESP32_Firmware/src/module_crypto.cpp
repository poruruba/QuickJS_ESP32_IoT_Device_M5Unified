#include <Arduino.h>
#include "main_config.h"

#ifdef _CRYPTO_ENABLE_

#include <mbedtls/md.h>
#include <mbedtls/aes.h>

#include "quickjs.h"
#include "module_crypto.h"
#include "lib_hmac.h"
#include <time.h>
#include "TOTP.h"
#include "module_utils.h"

static JSValue crypto_totpGenerate(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[0], (void**)&p_buffer, &unit_size, &unit_num);
  if( JS_IsNull(vbuffer) ){
    return JS_EXCEPTION;
  }
  if( unit_size != 1 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  time_t now = time(nullptr);
  if( now < 946652400 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }
  
  TOTP* totp = new TOTP(p_buffer, unit_num);
  char *code = totp->getCode(now);
  JS_FreeValue(ctx, vbuffer);

  JSValue value = JS_NewString(ctx, code);
  delete totp;

  return value;
}

static JSValue crypto_totpVerify(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[0], (void**)&p_buffer, &unit_size, &unit_num);
  if( JS_IsNull(vbuffer) ){
    return JS_EXCEPTION;
  }
  if( unit_size != 1 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  const char *p_code = JS_ToCString(ctx, argv[1]);
  if( p_code == NULL ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }  

  time_t now = time(nullptr);
  if( now < 946652400 ){
    JS_FreeValue(ctx, vbuffer);
    JS_FreeCString(ctx, p_code);
    return JS_EXCEPTION;
  }

  TOTP* totp = new TOTP(p_buffer, unit_num);
  char *code = totp->getCode(now);
  JS_FreeValue(ctx, vbuffer);

  bool result = (strcmp(code, p_code) == 0);
  JS_FreeCString(ctx, p_code);
  delete totp;

  return JS_NewBool(ctx, result);
}

static JSValue crypto_hmacCreate(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t type;
  JS_ToUint32(ctx, &type, argv[0]);
  
  if( type != HMAC_TYPE_SHA1 && type != HMAC_TYPE_MD5 && type != HMAC_TYPE_SHA256 )
    return JS_EXCEPTION;
  
  int output_len;
  if( type == HMAC_TYPE_SHA1 ){
    output_len = 20;
  }else if( type == HMAC_TYPE_MD5 ){
    output_len = 16;
  }else if( type == HMAC_TYPE_SHA256 ){
    output_len = 32;
  }else{
    return JS_EXCEPTION;
  }

  const char *p_key = JS_ToCString(ctx, argv[1]);
  if( p_key == NULL )
    return JS_EXCEPTION;
  const char *p_input = JS_ToCString(ctx, argv[2]);
  if( p_input == NULL ){
    JS_FreeCString(ctx, p_key);
    return JS_EXCEPTION;
  }

  uint8_t hmacResult[32];
  hmac_calculate(type, (const unsigned char*)p_key, strlen(p_key), (const unsigned char*)p_input, strlen(p_input), hmacResult);
  JS_FreeCString(ctx, p_key);
  JS_FreeCString(ctx, p_input);

  return JS_NewArrayBufferCopy(ctx, hmacResult, output_len);
}

static JSValue crypto_aesEcbEncrypt(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  uint32_t enc;
  JS_ToUint32(ctx, &enc, argv[0]);

  uint32_t type;
  JS_ToUint32(ctx, &type, argv[1]);
  if( type != 16 && type != 24 )
    return JS_EXCEPTION;

  JSValue jv = JS_GetPropertyStr(ctx, argv[2], "length");
  uint32_t key_len;
  JS_ToUint32(ctx, &key_len, jv);
  JS_FreeValue(ctx, jv);
  if( key_len < type )
    return JS_EXCEPTION;

  jv = JS_GetPropertyStr(ctx, argv[3], "length");
  uint32_t input_len;
  JS_ToUint32(ctx, &input_len, jv);
  JS_FreeValue(ctx, jv);
  if( (input_len % 16) != 0 )
    return JS_EXCEPTION;

  uint8_t key[24];
  uint8_t *p_input = (uint8_t*)malloc(input_len);
  if( p_input == NULL )
    return JS_EXCEPTION;
  uint8_t *p_output = (uint8_t*)malloc(input_len);
  if( p_output == NULL ){
    free(p_input);
    return JS_EXCEPTION;
  }

  for (uint32_t i = 0; i < type; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[2], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    key[i] = (uint8_t)value;
  }

  for (uint32_t i = 0; i < input_len; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[3], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    p_input[i] = (uint8_t)value;
  }

  mbedtls_aes_context context;
  mbedtls_aes_init(&context);
  mbedtls_aes_setkey_enc(&context, key, key_len * 8);
  for(long i = 0 ; i < input_len ; i += 16){
    mbedtls_aes_crypt_ecb(&context, enc, &p_input[i], &p_output[i]);
  }
  mbedtls_aes_free(&context);
  free(p_input);

  JSValue value = JS_EXCEPTION;
  value = JS_NewArrayBufferCopy(ctx, p_output, input_len);
  free(p_output);

  return value;
}

static JSValue crypto_aesCbcEncrypt(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  uint32_t enc;
  JS_ToUint32(ctx, &enc, argv[0]);

  uint32_t type;
  JS_ToUint32(ctx, &type, argv[1]);
  if( type != 16 && type != 24 )
    return JS_EXCEPTION;

  JSValue jv = JS_GetPropertyStr(ctx, argv[2], "length");
  uint32_t key_len;
  JS_ToUint32(ctx, &key_len, jv);
  JS_FreeValue(ctx, jv);
  if( key_len < type )
    return JS_EXCEPTION;

  jv = JS_GetPropertyStr(ctx, argv[3], "length");
  uint32_t input_len;
  JS_ToUint32(ctx, &input_len, jv);
  JS_FreeValue(ctx, jv);
  if( (input_len % 16) != 0 )
    return JS_EXCEPTION;

  jv = JS_GetPropertyStr(ctx, argv[4], "length");
  uint32_t iv_len;
  JS_ToUint32(ctx, &iv_len, jv);
  JS_FreeValue(ctx, jv);
  if( iv_len < 16 )
    return JS_EXCEPTION;

  uint8_t iv[16];
  uint8_t key[24];
  uint8_t *p_output = (uint8_t*)malloc(input_len);
  uint8_t *p_input = (uint8_t*)malloc(input_len);
  if( p_input == NULL )
    return JS_EXCEPTION;
  if( p_output == NULL ){
    free(p_input);
    return JS_EXCEPTION;
  }

  for (uint32_t i = 0; i < type; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[2], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    key[i] = (uint8_t)value;
  }

  for (uint32_t i = 0; i < input_len; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[3], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    p_input[i] = (uint8_t)value;
  }

  for (uint32_t i = 0; i < 16; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[4], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    iv[i] = (uint8_t)value;
  }

  mbedtls_aes_context context;

  mbedtls_aes_init(&context);
  mbedtls_aes_setkey_enc(&context, key, type * 8);
  mbedtls_aes_crypt_cbc(&context, enc, input_len, iv, p_input, p_output);
  mbedtls_aes_free(&context);
  free(p_input);

  JSValue value = JS_EXCEPTION;
  value = JS_NewArrayBufferCopy(ctx, p_output, input_len);
  free(p_output);

  return value;
}

static JSValue crypto_mdCreate(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  uint32_t type;
  JS_ToUint32(ctx, &type, argv[0]);
  if( type != 1 && type != 5 && type != 256 )
    return JS_EXCEPTION;
  
  int output_len;
  mbedtls_md_type_t md_type;
  if( type == 1 ){
    output_len = 20;
    md_type = MBEDTLS_MD_SHA1;
  }else if( type == 5 ){
    output_len = 16;
    md_type = MBEDTLS_MD_MD5;
  }else if( type == 256 ){
    output_len = 32;
    md_type = MBEDTLS_MD_SHA256;
  }

  JSValue jv = JS_GetPropertyStr(ctx, argv[1], "length");
  uint32_t input_len;
  JS_ToUint32(ctx, &input_len, jv);
  JS_FreeValue(ctx, jv);

  uint8_t *p_input = (uint8_t*)malloc(input_len);
  if( p_input == NULL )
    return JS_EXCEPTION;

  for (uint32_t i = 0; i < input_len; i++){
    JSValue jv = JS_GetPropertyUint32(ctx, argv[1], i);
    uint32_t value;
    JS_ToUint32(ctx, &value, jv);
    JS_FreeValue(ctx, jv);
    p_input[i] = (uint8_t)value;
  }

  uint8_t digestResult[32];
  mbedtls_md_context_t context;

  mbedtls_md_init(&context);
  mbedtls_md_setup(&context, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_update(&context, p_input, input_len);
	mbedtls_md_finish(&context, digestResult); // 32 or 16 bytes
  free(p_input);

  JSValue value = JS_EXCEPTION;
  value = JS_NewArrayBufferCopy(ctx, digestResult, output_len);

  return value;  
}

static const JSCFunctionListEntry crypto_funcs[] = {
    JSCFunctionListEntry{
        "totpGenerate", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, crypto_totpGenerate}
        }},
    JSCFunctionListEntry{
        "totpVerify", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, crypto_totpVerify}
        }},  
    JSCFunctionListEntry{
        "hmacCreate", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, crypto_hmacCreate}
        }},
    JSCFunctionListEntry{
        "aesEcbEncrypt", 0, JS_DEF_CFUNC, 0, {
          func : {4, JS_CFUNC_generic, crypto_aesEcbEncrypt}
        }},
    JSCFunctionListEntry{
        "aesCbcEncrypt", 0, JS_DEF_CFUNC, 0, {
          func : {5, JS_CFUNC_generic, crypto_aesCbcEncrypt}
        }},
    JSCFunctionListEntry{
        "mdCreate", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, crypto_mdCreate}
        }},
    JSCFunctionListEntry{
        "ENCRYPT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MBEDTLS_AES_ENCRYPT
        }},        
    JSCFunctionListEntry{
        "DECRYPT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : MBEDTLS_AES_DECRYPT
        }},
    JSCFunctionListEntry{
        "AES_KEY16", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 16
        }},        
    JSCFunctionListEntry{
        "AES_KEY24", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 24
        }},
    JSCFunctionListEntry{
        "MD_MD5", 0, JS_DEF_PROP_INT32, 0, {
          i32 : HMAC_TYPE_MD5
        }}, 
    JSCFunctionListEntry{
        "MD_SHA1", 0, JS_DEF_PROP_INT32, 0, {
          i32 : HMAC_TYPE_SHA1
        }},        
    JSCFunctionListEntry{
        "MD_SHA256", 0, JS_DEF_PROP_INT32, 0, {
          i32 : HMAC_TYPE_SHA256
        }},        
};

JSModuleDef *addModule_crypto(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Crypto", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, crypto_funcs,
                            sizeof(crypto_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, crypto_funcs,
        sizeof(crypto_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry crypto_module = {
  NULL,
  addModule_crypto,
  NULL,
  NULL
};

#endif
