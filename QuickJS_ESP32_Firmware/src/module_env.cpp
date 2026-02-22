#include <Arduino.h>
#include "main_config.h"

#ifdef _ENV_ENABLE_

#include "quickjs.h"
#include "module_env.h"

#ifdef ENV_MODULE_SHT30
#include "SHT3X.h"
static SHT3X sht30;
#endif
#ifdef ENV_MODULE_DHT12
#include "DHT12.h"
static DHT12 dht12;
#endif
#ifdef ENV_MODULE_SHT40
#include "Adafruit_SHT4x.h"
static Adafruit_SHT4x sht40 = Adafruit_SHT4x();
#endif

#ifdef ENV_MODULE_DHT12
static JSValue env_dht12_readTemperature(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  float tmp = dht12.readTemperature();
  return JS_NewFloat64(ctx, tmp);
}

static JSValue env_dht12_readHumidity(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  float hum = dht12.readHumidity();
  return JS_NewFloat64(ctx, hum);
}
#endif

#ifdef ENV_MODULE_SHT30
static JSValue env_sht30_get(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if(sht30.get() != 0)
    return JS_EXCEPTION;
  
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "cTemp", JS_NewFloat64(ctx, sht30.cTemp));
  JS_SetPropertyStr(ctx, obj, "fTemp", JS_NewFloat64(ctx, sht30.fTemp));
  JS_SetPropertyStr(ctx, obj, "humidity", JS_NewFloat64(ctx, sht30.humidity));
  return obj;
}
#endif

#ifdef ENV_MODULE_SHT40
static JSValue env_sht40_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !sht40.begin() )
    return JS_EXCEPTION;

  sht40.setPrecision(SHT4X_HIGH_PRECISION);
  sht40.setHeater(SHT4X_NO_HEATER);

  return JS_UNDEFINED;
}

static JSValue env_sht40_setConfig(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t precision, heater;
  JS_ToInt32(ctx, &precision, argv[0]);
  JS_ToInt32(ctx, &heater, argv[1]);

  sht40.setPrecision((sht4x_precision_t)precision);
  sht40.setHeater((sht4x_heater_t)heater);

  return JS_UNDEFINED;
}

static JSValue env_sht40_get(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  sensors_event_t humidity, temp;
  if(!sht40.getEvent(&humidity, &temp))
    return JS_EXCEPTION;
  
  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "temperature", JS_NewFloat64(ctx, temp.temperature));
  JS_SetPropertyStr(ctx, obj, "humidity", JS_NewFloat64(ctx, humidity.relative_humidity));
  return obj;
}
#endif

static const JSCFunctionListEntry env_funcs[] = {
#ifdef ENV_MODULE_DHT12
    JSCFunctionListEntry{
        "dht12_readTemperature", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, env_dht12_readTemperature}
        }},
    JSCFunctionListEntry{
        "dht12_readHumidity", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, env_dht12_readHumidity}
        }},
#endif
#ifdef ENV_MODULE_SHT30
    JSCFunctionListEntry{
        "sht30_get", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, env_sht30_get}
        }},
#endif
#ifdef ENV_MODULE_SHT40
    JSCFunctionListEntry{
        "sht40_begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, env_sht40_begin}
        }},
    JSCFunctionListEntry{
        "sht40_setConfig", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, env_sht40_setConfig}
        }},
    JSCFunctionListEntry{
        "sht40_get", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, env_sht40_get}
        }},
    JSCFunctionListEntry{
        "SHT4X_HIGH_PRECISION", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_HIGH_PRECISION
        }},
    JSCFunctionListEntry{
        "SHT4X_MED_PRECISION", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_MED_PRECISION
        }},
    JSCFunctionListEntry{
        "SHT4X_LOW_PRECISION", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_LOW_PRECISION
        }},
    JSCFunctionListEntry{
        "SHT4X_NO_HEATER", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_NO_HEATER
        }},
    JSCFunctionListEntry{
        "SHT4X_HIGH_HEATER_1S", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_HIGH_HEATER_1S
        }},
    JSCFunctionListEntry{
        "SHT4X_HIGH_HEATER_100MS", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_HIGH_HEATER_100MS
        }},
    JSCFunctionListEntry{
        "SHT4X_MED_HEATER_1S", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_MED_HEATER_1S
        }},
    JSCFunctionListEntry{
        "SHT4X_MED_HEATER_100MS", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_MED_HEATER_100MS
        }},
    JSCFunctionListEntry{
        "SHT4X_LOW_HEATER_1S", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_LOW_HEATER_1S
        }},
    JSCFunctionListEntry{
        "SHT4X_LOW_HEATER_100MS", 0, JS_DEF_PROP_INT32, 0, {
          i32 : SHT4X_LOW_HEATER_100MS
        }},
#endif
};

JSModuleDef *addModule_env(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Env", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, env_funcs,
                            sizeof(env_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, env_funcs,
        sizeof(env_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry env_module = {
  NULL,
  addModule_env,
  NULL,
  NULL
};

#endif
