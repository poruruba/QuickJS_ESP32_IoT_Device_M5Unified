#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_AIRQUALITY_ENABLE_

#include "quickjs.h"
#include "module_unit_airquality.h"
#include "AirQuality.h"

static AirQuality airquality;

static JSValue unit_airquality_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t pin;
  JS_ToUint32(ctx, &pin, argv[0]);

  airquality.init(pin);

  return JS_UNDEFINED;
}

static JSValue unit_airquality_isReady(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  bool value = airquality.isReady();
  return JS_NewBool(ctx, value);
}

static JSValue unit_airquality_slope(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int value = airquality.slope();
  return JS_NewInt32(ctx, value);
}

static JSValue unit_airquality_update(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  airquality.update();
  return JS_UNDEFINED;
}

static JSValue unit_airquality_end(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  airquality.stopTimer();
  return JS_UNDEFINED;
}

static JSValue unit_airquality_reset(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  airquality.resetAll();
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry unit_airquality_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_airquality_begin}
        }},
    JSCFunctionListEntry{
        "isReady", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_airquality_isReady}
        }},
    JSCFunctionListEntry{
        "slope", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_airquality_slope}
        }},
    JSCFunctionListEntry{
        "update", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_airquality_update}
        }},
    JSCFunctionListEntry{
        "end", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_airquality_end}
        }},
    JSCFunctionListEntry{
        "reset", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_airquality_reset}
        }},
};

JSModuleDef *addModule_unit_airquality(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitAirquality", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_airquality_funcs,
                            sizeof(unit_airquality_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_airquality_funcs,
        sizeof(unit_airquality_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_unit_airquality(void)
{
  airquality.stopTimer();
}

JsModuleEntry unit_airquality_module = {
  "UnitAirquality",
  NULL,
  addModule_unit_airquality,
  NULL,
  endModule_unit_airquality
};

#endif