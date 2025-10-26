#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_ASR_ENABLE_

#include "quickjs.h"
#include "module_unit_asr.h"
#include "unit_asr.hpp"

static ASRUnit asr;
static bool isBegan = false;

static JSValue unit_asr_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t pin_rx, pin_tx;
  JS_ToUint32(ctx, &pin_rx, argv[0]);
  JS_ToUint32(ctx, &pin_tx, argv[1]);

  if( isBegan ){
    Serial1.end();
    isBegan = false;
  }
  asr.begin(&Serial1, UNIT_ASR_BAUD, pin_tx, pin_rx);
  isBegan = true;

  return JS_UNDEFINED;
}

static JSValue unit_asr_end(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isBegan ){
    Serial1.end();
    isBegan = false;
  }

  return JS_UNDEFINED;
}

static JSValue unit_asr_sendCommandNum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);

  asr.sendComandNum(num);

  return JS_UNDEFINED;
}

static JSValue unit_asr_addCommandWord(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);
  const char *word = JS_ToCString(ctx, argv[1]);

  bool ret = asr.addCommandWord(num, word, nullptr);
  JS_FreeCString(ctx, word);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_asr_removeCommandWord(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *word = JS_ToCString(ctx, argv[0]);

  bool ret = asr.removeCommandWord(word);
  JS_FreeCString(ctx, word);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_asr_searchCommandNum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *word = JS_ToCString(ctx, argv[0]);

  int8_t ret = asr.searchCommandNum(word);
  JS_FreeCString(ctx, word);

  return JS_NewInt32(ctx, ret);
}

static JSValue unit_asr_searchCommandWord(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);

  String ret = asr.searchCommandWord(num);

  return JS_NewString(ctx, ret.c_str());
}

static JSValue unit_asr_update(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool ret = asr.update();

  if( !ret )
    return JS_UNDEFINED;

  JSValue obj = JS_NewObject(ctx);
  uint8_t num = asr.getCurrentCommandNum();
  String word = asr.getCurrentCommandWord();
  JS_SetPropertyStr(ctx, obj, "commandNum", JS_NewUint32(ctx, num));
  JS_SetPropertyStr(ctx, obj, "commandWord", JS_NewString(ctx, word.c_str()));
  return obj;
}

static const JSCFunctionListEntry unit_asr_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, unit_asr_begin}
        }},
    JSCFunctionListEntry{
        "end", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_asr_end}
        }},
    JSCFunctionListEntry{
        "sendCommandNum", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_asr_sendCommandNum}
        }},
    JSCFunctionListEntry{
        "addCommandWord", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, unit_asr_addCommandWord}
        }},
    JSCFunctionListEntry{
        "removeCommandWord", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_asr_removeCommandWord}
        }},
    JSCFunctionListEntry{
        "searchCommandNum", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_asr_searchCommandNum}
        }},
    JSCFunctionListEntry{
        "searchCommandWord", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_asr_searchCommandWord}
        }},
    JSCFunctionListEntry{
        "update", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_asr_update}
        }},
};

JSModuleDef *addModule_unit_asr(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitAsr", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_asr_funcs,
                            sizeof(unit_asr_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_asr_funcs,
        sizeof(unit_asr_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_asr(void)
{
  if( isBegan ){
    Serial1.end();
    isBegan = false;
  }
}

JsModuleEntry unit_asr_module = {
  NULL,
  addModule_unit_asr,
  NULL,
  endModule_asr
};

#endif