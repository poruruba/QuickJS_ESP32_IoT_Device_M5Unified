#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_STEP16_ENABLE_

#include <Wire.h>
#include "quickjs.h"
#include "module_unit_step16.h"
#include "unit_step16.hpp"

static UnitStep16 step16;

static JSValue unit_step16_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool ret = step16.begin();

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getValue(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t value = step16.getValue();

  return JS_NewUint32(ctx, value);
}

static JSValue unit_step16_setLedConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t config;
  JS_ToUint32(ctx, &config, argv[0]);

  bool ret = step16.setLedConfig(config);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getLedConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t config = step16.getLedConfig();

  return JS_NewUint32(ctx, config);
}

static JSValue unit_step16_setLedBrightness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t brightness;
  JS_ToUint32(ctx, &brightness, argv[0]);

  bool ret = step16.setLedBrightness(brightness);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getLedBrightness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t config = step16.getLedBrightness();

  return JS_NewUint32(ctx, config);
}

static JSValue unit_step16_setSwitchState(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t state;
  JS_ToUint32(ctx, &state, argv[0]);

  bool ret = step16.setSwitchState(state);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getSwitchState(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t state = step16.getSwitchState();

  return JS_NewUint32(ctx, state);
}

static JSValue unit_step16_setRgbConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t config;
  JS_ToUint32(ctx, &config, argv[0]);

  bool ret = step16.setRgbConfig(config);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getRgbConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t config = step16.getRgbConfig();

  return JS_NewUint32(ctx, config);
}

static JSValue unit_step16_setRgbBrightness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t brightness;
  JS_ToUint32(ctx, &brightness, argv[0]);

  bool ret = step16.setRgbBrightness(brightness);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_getRgbBrightness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t config = step16.getRgbBrightness();

  return JS_NewUint32(ctx, config);
}

static JSValue unit_step16_setRgb(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t r, g, b;
  JS_ToUint32(ctx, &r, argv[0]);
  JS_ToUint32(ctx, &g, argv[1]);
  JS_ToUint32(ctx, &b, argv[2]);

  bool ret = step16.setRgb(r, g, b);
  if( !ret )
    return JS_EXCEPTION;

  return JS_NewUint32(ctx, ret);
}

static JSValue unit_step16_getRgb(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t r, g, b;

  uint8_t ret = step16.getRgb(&r, &g, &b);
  if( !ret )
    return JS_EXCEPTION;

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "r", JS_NewUint32(ctx, r));
  JS_SetPropertyStr(ctx, obj, "g", JS_NewUint32(ctx, g));
  JS_SetPropertyStr(ctx, obj, "b", JS_NewUint32(ctx, b));
  return obj;
}

static JSValue unit_step16_saveToFlash(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t save;
  JS_ToUint32(ctx, &save, argv[0]);

  bool ret = step16.saveToFlash(save);

  return JS_NewBool(ctx, ret);
}

static JSValue unit_step16_setDefaultConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t ret = step16.setDefaultConfig();

  return JS_NewUint32(ctx, ret);
}

static const JSCFunctionListEntry unit_step16_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_begin}
        }},
    JSCFunctionListEntry{
        "getValue", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getValue}
        }},
    JSCFunctionListEntry{
        "setLedConfig", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_setLedConfig}
        }},
    JSCFunctionListEntry{
        "getLedConfig", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getLedConfig}
        }},
    JSCFunctionListEntry{
        "setLedBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_setLedBrightness}
        }},
    JSCFunctionListEntry{
        "getLedBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getLedBrightness}
        }},
    JSCFunctionListEntry{
        "setSwitchState", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_setSwitchState}
        }},
    JSCFunctionListEntry{
        "getSwitchState", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getSwitchState}
        }},
    JSCFunctionListEntry{
        "setRgbConfig", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_setRgbConfig}
        }},
    JSCFunctionListEntry{
        "getRgbConfig", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getRgbConfig}
        }},
    JSCFunctionListEntry{
        "setRgbBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_setRgbBrightness}
        }},
    JSCFunctionListEntry{
        "getRgbBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getRgbBrightness}
        }},
    JSCFunctionListEntry{
        "setRgb", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, unit_step16_setRgb}
        }},
    JSCFunctionListEntry{
        "getRgb", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_getRgb}
        }},
    JSCFunctionListEntry{
        "saveToFlash", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_step16_saveToFlash}
        }},
    JSCFunctionListEntry{
        "setDefaultConfig", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_step16_setDefaultConfig}
        }},
    JSCFunctionListEntry{
        "SAVE_CONFIG_LED", 0, JS_DEF_PROP_INT32, 0, {
          i32 : UNIT_STEP16_SAVE_LED_CONFIG
        }},
    JSCFunctionListEntry{
        "SAVE_CONFIG_RGB", 0, JS_DEF_PROP_INT32, 0, {
          i32 : UNIT_STEP16_SAVE_RGB_CONFIG
        }},
};

JSModuleDef *addModule_unit_step16(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitStep16", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_step16_funcs,
                            sizeof(unit_step16_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_step16_funcs,
        sizeof(unit_step16_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry unit_step16_module = {
  NULL,
  addModule_unit_step16,
  NULL,
  NULL
};

#endif