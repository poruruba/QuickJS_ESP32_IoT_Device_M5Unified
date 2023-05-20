#include <Arduino.h>
#include "main_config.h"

#ifdef _IMU_ENABLE_

#include "quickjs.h"
#include "module_imu.h"

static JSValue esp32_imu_getAccel(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  JSValue obj = JS_NewObject(ctx);
  float ax, ay, az;
  M5.Imu.getAccel(&ax, &ay, &az);
  JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, ax));
  JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, ay));
  JS_SetPropertyStr(ctx, obj, "z", JS_NewFloat64(ctx, az));
  return obj;
}

static JSValue esp32_imu_getGyro(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  JSValue obj = JS_NewObject(ctx);
  float gx, gy, gz;
  M5.Imu.getGyro(&gx, &gy, &gz);
  JS_SetPropertyStr(ctx, obj, "x", JS_NewFloat64(ctx, gx));
  JS_SetPropertyStr(ctx, obj, "y", JS_NewFloat64(ctx, gy));
  JS_SetPropertyStr(ctx, obj, "z", JS_NewFloat64(ctx, gz));
  return obj;
}

static JSValue esp32_imu_getTemp(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  float t;
  M5.Imu.getTemp(&t);
  return JS_NewFloat64(ctx, t);
}

static const JSCFunctionListEntry imu_funcs[] = {
    JSCFunctionListEntry{
        "getAccel", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, esp32_imu_getAccel}
        }},
    JSCFunctionListEntry{
        "getGyro", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, esp32_imu_getGyro}
        }},
    JSCFunctionListEntry{
        "getTemp", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, esp32_imu_getTemp}
        }},
};

JSModuleDef *addModule_imu(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Imu", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, imu_funcs,
                            sizeof(imu_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, imu_funcs,
        sizeof(imu_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initialize_imu(void){
  M5.Imu.begin();

  return 0;
}

JsModuleEntry imu_module = {
  initialize_imu,
  addModule_imu,
  NULL,
  NULL
};

#endif
