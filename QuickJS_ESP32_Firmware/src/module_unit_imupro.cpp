#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_IMUPRO_ENABLE_

#include "quickjs.h"
#include "module_unit_imupro.h"
#include "M5_IMU_PRO.h"
#include "MadgwickAHRS.h"

static Madgwick filter;

#define BIM270_SENSOR_ADDR 0x68
#define BMP280_SENSOR_ADDR 0x76

static BMI270::BMI270 bmi270;
static Adafruit_BMP280 bmp(&Wire);

static float roll  = 0;
static float pitch = 0;
static float yaw   = 0;

static JSValue unit_imupro_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    unsigned status = bmp.begin(BMP280_SENSOR_ADDR);
    if (!status) {
        Serial.println(
            F("Could not find a valid BMP280 sensor, check wiring or "
              "try a different address!"));
        Serial.print("SensorID was: 0x");
        Serial.println(bmp.sensorID(), 16);
        return JS_EXCEPTION;
    }

    int ret = bmi270.init(I2C_NUM_0, BIM270_SENSOR_ADDR);
    if( !ret ){
        Serial.println("bmi270 not found");
        return JS_EXCEPTION;
    }
        
    filter.begin(20);  // 20hz

    return JS_UNDEFINED;
}

static JSValue unit_imupro_readAcceleration(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    if (!bmi270.accelerationAvailable())
        return JS_NULL;

    float ax, ay, az;
    int ret = bmi270.readAcceleration(ax, ay, az);
    if( !ret )
        return JS_EXCEPTION;
    
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "ax", JS_NewFloat64(ctx, ax));
    JS_SetPropertyStr(ctx, obj, "ay", JS_NewFloat64(ctx, ay));
    JS_SetPropertyStr(ctx, obj, "az", JS_NewFloat64(ctx, az));

    return obj;
}

static JSValue unit_imupro_readGyroscope(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    if (!bmi270.gyroscopeAvailable())
        return JS_NULL;

    float gx, gy, gz;
    int ret = bmi270.readGyroscope(gx, gy, gz);
    if( !ret )
        return JS_EXCEPTION;
    
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "gx", JS_NewFloat64(ctx, gx));
    JS_SetPropertyStr(ctx, obj, "gy", JS_NewFloat64(ctx, gy));
    JS_SetPropertyStr(ctx, obj, "gz", JS_NewFloat64(ctx, gz));

    return obj;
}

static JSValue unit_imupro_readMagneticField(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    if (!bmi270.magneticFieldAvailable())
        return JS_NULL;

    int16_t mx, my, mz = 0;
    int ret = bmi270.readMagneticField(mx, my, mz);
    if( !ret )
        return JS_EXCEPTION;
    
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "mx", JS_NewInt32(ctx, mx));
    JS_SetPropertyStr(ctx, obj, "my", JS_NewInt32(ctx, my));
    JS_SetPropertyStr(ctx, obj, "mz", JS_NewInt32(ctx, mz));

    return obj;
}

static JSValue unit_imupro_readTemperature(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    float temp = bmp.readTemperature();

    return JS_NewFloat64(ctx, temp);
}

static JSValue unit_imupro_readPressure(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    float press = bmp.readPressure();

    return JS_NewFloat64(ctx, press);
}

static JSValue unit_imupro_readAltitude(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    float alt = bmp.readAltitude(1013.25);

    return JS_NewFloat64(ctx, alt);
}

static JSValue unit_imupro_updateImu(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
    if (!bmi270.accelerationAvailable() || !bmi270.gyroscopeAvailable())
        return JS_NULL;

    float ax, ay, az;
    float gx, gy, gz;
    int ret;
    ret = bmi270.readAcceleration(ax, ay, az);
    if( !ret )
        return JS_EXCEPTION;
    ret = bmi270.readGyroscope(gx, gy, gz);
    if( !ret )
        return JS_EXCEPTION;
    filter.updateIMU(gx, gy, gz, ax, ay, az);
    roll  = filter.getRoll();
    pitch = filter.getPitch();
    yaw   = filter.getYaw();
    
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "roll", JS_NewFloat64(ctx, roll));
    JS_SetPropertyStr(ctx, obj, "pitch", JS_NewFloat64(ctx, pitch));
    JS_SetPropertyStr(ctx, obj, "yaw", JS_NewFloat64(ctx, yaw));

    return obj;
}

static const JSCFunctionListEntry unit_imupro_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_begin}
        }},
    JSCFunctionListEntry{
        "readAcceleration", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readAcceleration}
        }},
    JSCFunctionListEntry{
        "readGyroscope", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readGyroscope}
        }},
    JSCFunctionListEntry{
        "readMagneticField", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readMagneticField}
        }},
    JSCFunctionListEntry{
        "readTemperature", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readTemperature}
        }},
    JSCFunctionListEntry{
        "readPressure", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readPressure}
        }},
    JSCFunctionListEntry{
        "readAltitude", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_readAltitude}
        }},
    JSCFunctionListEntry{
        "updateImu", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_imupro_updateImu}
        }},

};

JSModuleDef *addModule_unit_imupro(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitImuPro", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_imupro_funcs,
                            sizeof(unit_imupro_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_imupro_funcs,
        sizeof(unit_imupro_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry unit_imupro_module = {
  NULL,
  addModule_unit_imupro,
  NULL,
  NULL
};

#endif
