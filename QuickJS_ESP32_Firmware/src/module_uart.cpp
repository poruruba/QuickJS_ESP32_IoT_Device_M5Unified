#include <Arduino.h>
#include "quickjs.h"
#include "module_type.h"
#include "module_utils.h"

static JSValue esp32_uart_begin(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv, int magic)
{
  uint32_t baud, pin_rx, pin_tx;
  JS_ToUint32(ctx, &baud, argv[0]);
  JS_ToUint32(ctx, &pin_rx, argv[1]);
  JS_ToUint32(ctx, &pin_tx, argv[2]);

  Serial1.begin(baud, SERIAL_8N1, pin_rx, pin_tx);

  return JS_UNDEFINED;
}

static JSValue esp32_uart_available(JSContext *ctx, JSValueConst jsThis,
                                    int argc, JSValueConst *argv, int magic)
{
  return JS_NewInt32(ctx, Serial1.available());
}

static JSValue esp32_uart_write(JSContext *ctx, JSValueConst jsThis,
                                int argc, JSValueConst *argv, int magic)
{
  int tag = JS_VALUE_GET_TAG(argv[0]);
  if (tag == JS_TAG_INT){
    uint32_t value;
    JS_ToUint32(ctx, &value, argv[0]);

    return JS_NewInt32(ctx, Serial1.write((uint8_t)value));
  }else{
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

    size_t ret = Serial1.write(p_buffer, unit_num);
    JS_FreeValue(ctx, vbuffer);

    return JS_NewInt32(ctx, ret);
  }
}

static JSValue esp32_uart_read(JSContext *ctx, JSValueConst jsThis,
                               int argc, JSValueConst *argv, int magic)
{
  if (argc > 0){
    uint32_t length;
    JS_ToUint32(ctx, &length, argv[0]);
    uint8_t *p_buffer = (uint8_t*)malloc(length);
    if( p_buffer == NULL )
      return JS_EXCEPTION;
    uint32_t i;
    for (i = 0; i < length; i++){
      int c = Serial1.read();
      if( c < 0 )
        break;
      p_buffer[i] = (uint8_t)c;
    }

    JSValue value = JS_NewArrayBufferCopy(ctx, p_buffer, i);
    free(p_buffer);
    return value;
  }else{
    return JS_NewInt32(ctx, Serial1.read());
  }
}

static JSValue esp32_uart_end(JSContext *ctx, JSValueConst jsThis,
                                    int argc, JSValueConst *argv, int magic)
{
  Serial1.end();

  return JS_UNDEFINED;
}

static JSValue esp32_uart_flush(JSContext *ctx, JSValueConst jsThis,
                                    int argc, JSValueConst *argv, int magic)
{
  Serial1.flush();

  return JS_UNDEFINED;
}

static JSValue esp32_uart_peek(JSContext *ctx, JSValueConst jsThis,
                                    int argc, JSValueConst *argv, int magic)
{
  int ret = Serial1.peek();

  return JS_NewInt32(ctx, ret);
}

static JSValue esp32_uart_setTimeout(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv, int magic)
{
  uint32_t tmout;

  JS_ToUint32(ctx, &tmout, argv[0]);
  Serial1.setTimeout(tmout);

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry uart_funcs[] = {
    JSCFunctionListEntry{"begin", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_begin}}
                        }},
    JSCFunctionListEntry{"available", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_available}}
                         }},
    JSCFunctionListEntry{"write", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_write}}
                        }},
    JSCFunctionListEntry{"read", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_read}}
                         }},
    JSCFunctionListEntry{"end", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_end}}
                        }},
    JSCFunctionListEntry{"flush", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_flush}}
                         }},
    JSCFunctionListEntry{"peek", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_peek}}
                        }},
    JSCFunctionListEntry{"setTimeout", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_uart_setTimeout}}
                         }},
};

JSModuleDef *addModule_uart(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Uart", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, uart_funcs,
                            sizeof(uart_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, uart_funcs,
        sizeof(uart_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_uart(void){
  Serial1.end();
}

JsModuleEntry uart_module = {
  NULL,
  addModule_uart,
  NULL,
  endModule_uart
};
