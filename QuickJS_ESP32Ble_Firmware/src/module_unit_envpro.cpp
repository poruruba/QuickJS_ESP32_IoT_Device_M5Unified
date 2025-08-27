#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_ENVPRO_ENABLE_

#include "quickjs.h"
#include "module_unit_envpro.h"
#include <Adafruit_BME680.h>

#define SEALEVELPRESSURE_HPA (1013.25)
static Adafruit_BME680 *bme;

static JSValue unit_envpro_begin(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  if(bme != NULL )
    delete bme;
  bme = new Adafruit_BME680(&Wire);

  if( !bme->begin() )
    return JS_EXCEPTION;

  bme->setTemperatureOversampling(BME680_OS_8X);
  bme->setHumidityOversampling(BME680_OS_2X);
  bme->setPressureOversampling(BME680_OS_4X);
  bme->setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme->setGasHeater(320, 150);

  return JS_UNDEFINED;
}

static JSValue unit_envpro_read(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  unsigned long endTime = bme->beginReading();
  if (endTime == 0)
    return JS_EXCEPTION;

  delay(50);

  if (!bme->endReading())
    return JS_EXCEPTION;

  JSValue obj = JS_NewObject(ctx);
  float temperature = bme->temperature;
  JS_SetPropertyStr(ctx, obj, "temperature", JS_NewFloat64(ctx, temperature));
  float humidity = bme->humidity;
  JS_SetPropertyStr(ctx, obj, "humidity", JS_NewFloat64(ctx, humidity));
  uint32_t pressure = bme->pressure;
  JS_SetPropertyStr(ctx, obj, "pressure", JS_NewFloat64(ctx, pressure / 100.0));
  uint32_t gas = bme->gas_resistance;
  JS_SetPropertyStr(ctx, obj, "gas", JS_NewFloat64(ctx, gas / 1000.0));
  float altitude = bme->readAltitude(SEALEVELPRESSURE_HPA);
  JS_SetPropertyStr(ctx, obj, "altitude", JS_NewFloat64(ctx, altitude));

  return obj;
}

static const JSCFunctionListEntry unit_envpro_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_envpro_begin}
        }},
    JSCFunctionListEntry{
        "read", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_envpro_read}
        }},
};

JSModuleDef *addModule_unit_envpro(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitEnvPro", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_envpro_funcs,
                            sizeof(unit_envpro_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_envpro_funcs,
        sizeof(unit_envpro_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry unit_envpro_module = {
  NULL,
  addModule_unit_envpro,
  NULL,
  NULL
};

#endif
