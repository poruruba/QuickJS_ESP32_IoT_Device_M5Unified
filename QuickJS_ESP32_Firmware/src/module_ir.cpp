#include <Arduino.h>
#include "main_config.h"

#ifdef _IR_ENABLE_

#include "quickjs.h"
#include "module_type.h"
#include "module_utils.h"

#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

#define IR_DEFAULT_HZ 38

static bool g_receiving = false;
static IRsend *g_irsend = NULL;
static IRrecv *g_irrecv = NULL;
static decode_results results;

static JSValue esp32_ir_sendBegin(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  uint32_t pin;
  JS_ToUint32(ctx, &pin, argv[0]);

  if( g_irsend != NULL ){
    delete g_irsend;
    g_irsend = NULL;
  }

  g_irsend = new IRsend(pin);
  g_irsend->begin();

  return JS_UNDEFINED;
}

static JSValue esp32_ir_send(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irsend == NULL )
    return JS_EXCEPTION;

  uint32_t data;
  JS_ToUint32(ctx, &data, argv[0]);
  uint32_t repeat = 0;
  if( argc >= 2 )
    JS_ToUint32(ctx, &repeat, argv[1]);

  if( g_receiving )
    g_irrecv->disableIRIn();
  g_irsend->sendNEC(data, 32, repeat);
  if( g_receiving )
    g_irrecv->enableIRIn();

  return JS_UNDEFINED;
}

static JSValue esp32_ir_sendRaw(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irsend == NULL )
    return JS_EXCEPTION;

  uint16_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[0], (void**)&p_buffer, &unit_size, &unit_num);
  if( JS_IsNull(vbuffer) )
    return JS_EXCEPTION;
  if( unit_size != 2 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  if( g_receiving )
    g_irrecv->disableIRIn();
  g_irsend->sendRaw(p_buffer, unit_num, IR_DEFAULT_HZ);
  if( g_receiving )
    g_irrecv->enableIRIn();

  JS_FreeValue(ctx, vbuffer);

  return JS_UNDEFINED;
}

static JSValue esp32_ir_recvBegin(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  uint32_t pin;
  JS_ToUint32(ctx, &pin, argv[0]);

  if( g_irrecv != NULL ){
    delete g_irrecv;
    g_irrecv = NULL;
  }

  g_irrecv = new IRrecv(pin);
  g_irrecv->enableIRIn();
  g_irrecv->disableIRIn();
  g_receiving = false;

  return JS_UNDEFINED;
}

static JSValue esp32_ir_recvStart(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irrecv == NULL )
    return JS_EXCEPTION;
  
  g_irrecv->enableIRIn();
  g_irrecv->resume();
  g_receiving = true;

  return JS_UNDEFINED;
}

static JSValue esp32_ir_recvStop(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irrecv == NULL )
    return JS_EXCEPTION;

  g_irrecv->disableIRIn();
  g_receiving = false;

  return JS_UNDEFINED;
}

static JSValue esp32_ir_checkRecv(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irrecv == NULL )
    return JS_EXCEPTION;

  uint32_t type = NEC;
  if( argc >= 1 )
    JS_ToUint32(ctx, &type, argv[0]);

  if( g_irrecv->decode(&results) ){
    if( results.decode_type == type ){
      uint64_t value = results.value;
      g_irrecv->resume();
      return JS_NewUint32(ctx, value);
    }else{
      g_irrecv->resume();
      return JS_NewUint32(ctx, 0);
    }
  }

  return JS_NewUint32(ctx, 0);
}

static JSValue esp32_ir_checkRecvTyped(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irrecv == NULL )
    return JS_EXCEPTION;

  if( g_irrecv->decode(&results) ){
    uint64_t value = results.value;
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "type", JS_NewUint32(ctx, results.decode_type));
    JS_SetPropertyStr(ctx, obj, "value", JS_NewUint32(ctx, value));
    JS_SetPropertyStr(ctx, obj, "value_high", JS_NewUint32(ctx, value >> 32));
    JS_SetPropertyStr(ctx, obj, "bits", JS_NewUint32(ctx, results.bits));
    g_irrecv->resume();
    return obj;
  }

  return JS_UNDEFINED;
}

static JSValue esp32_ir_checkRecvRaw(JSContext *ctx, JSValueConst jsThis, int argc,
                               JSValueConst *argv)
{
  if( g_irrecv == NULL )
    return JS_EXCEPTION;

  if( g_irrecv->decode(&results) ){
    uint16_t * result = resultToRawArray(&results);
    uint16_t len = getCorrectedRawLength(&results);
    JSValue array = JS_NewArray(ctx);
    for (uint16_t i = 0; i < len; i++)
      JS_SetPropertyUint32(ctx, array, i, JS_NewInt32(ctx, result[i]));
    delete[] result;
    g_irrecv->resume();

    return array;
  }

  return JS_NULL;
}

static const JSCFunctionListEntry ir_funcs[] = {
    JSCFunctionListEntry{"sendBegin", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_ir_sendBegin}
                         }},
    JSCFunctionListEntry{"send", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_ir_send}
                         }},
    JSCFunctionListEntry{"sendRaw", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_ir_sendRaw}
                         }},
    JSCFunctionListEntry{"recvBegin", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_ir_recvBegin}
                         }},
    JSCFunctionListEntry{"recvStart", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_ir_recvStart}
                         }},
    JSCFunctionListEntry{"recvStop", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_ir_recvStop}
                         }},
    JSCFunctionListEntry{"checkRecv", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_ir_checkRecv}
                         }},
    JSCFunctionListEntry{"checkRecvTyped", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_ir_checkRecvTyped}
                         }},
    JSCFunctionListEntry{"checkRecvRaw", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_ir_checkRecvRaw}
                         }},
    JSCFunctionListEntry{
        "TYPE_NEC", 0, JS_DEF_PROP_INT32, 0, {
          i32 : NEC
        }},
    JSCFunctionListEntry{
        "TYPE_SONY", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SONY
        }},                                 
};

JSModuleDef *addModule_ir(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Ir", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, ir_funcs,
                            sizeof(ir_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, ir_funcs,
        sizeof(ir_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_ir(void){
  if( g_irsend != NULL ){
    delete g_irsend;
    g_irsend = NULL;
  }
  if( g_irrecv != NULL ){
    delete g_irrecv;
    g_irrecv = NULL;
  }
  g_receiving = false;
}

JsModuleEntry ir_module = {
  NULL,
  addModule_ir,
  NULL,
  endModule_ir
};

#endif