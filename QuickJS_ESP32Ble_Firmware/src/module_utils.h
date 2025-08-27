#ifndef _MODULE_UTILS_H_
#define _MODULE_UTILS_H_

#include <ArduinoJson.h>
#include "quickjs.h"
#include "module_type.h"

extern JsModuleEntry utils_module;

long http_get(String url, String *response);
long http_get_binary(String url, uint8_t *p_buffer, unsigned long *p_len);
long http_get_json(String url, JsonDocument *doc);
uint8_t *http_get_binary2(const char *url, uint32_t *p_len);

unsigned long b64_encode_length(unsigned long input_length);
unsigned long b64_encode(const unsigned char input[], unsigned long input_length, char output[]);
unsigned long b64_decode_length(const char input[]);
unsigned long b64_decode(const char input[], unsigned char output[]);

JSValue getTypedArrayBuffer(JSContext *ctx, JSValue value, void** p_buffer, uint8_t *p_unit_size, uint32_t *p_unit_num);
long getNumberArray(JSContext *ctx, JSValue value, int32_t **pp_buffer, uint32_t *p_length);
JSValue createNumberArray(JSContext *ctx, int32_t *p_buffer, uint32_t unit_num);

#endif