#ifndef _MODULE_UTILS_H_
#define _MODULE_UTILS_H_

#include "quickjs.h"
#include "module_type.h"

extern JsModuleEntry utils_module;

String http_get(const char* url);
uint8_t *http_get_binary2(const char *url, uint32_t *p_len);
#if 0
// long http_get_binary(String url, uint8_t *p_buffer, unsigned long *p_len);
#endif

unsigned long b64_encode_length(unsigned long input_length);
unsigned long b64_encode(const unsigned char input[], unsigned long input_length, char output[]);
unsigned long b64_decode_length(const char input[]);
unsigned long b64_decode(const char input[], unsigned char output[]);
String urlencode(String str);

JSValue getBinaryFromTypedArray(JSContext *ctx, JSValue value, void** pp_buffer, uint8_t *p_unit_size, uint32_t *p_unit_num);
JSValue getTypedArrayBuffer(JSContext *ctx, JSValue value, void** p_buffer, uint8_t *p_unit_size, uint32_t *p_unit_num);
long getNumberArray(JSContext *ctx, JSValue value, int32_t **pp_buffer, uint32_t *p_length);
JSValue createNumberArray(JSContext *ctx, int32_t *p_buffer, uint32_t unit_num);
JSValue create_Uint8Array(JSContext *ctx, const uint8_t *p_buffer, uint32_t len);
JSValue from_Uint8Array(JSContext *ctx, JSValue value, uint8_t** pp_buffer, uint32_t *p_num);

void my_mem_free(JSRuntime *rt, void *opaque, void *ptr);

#endif