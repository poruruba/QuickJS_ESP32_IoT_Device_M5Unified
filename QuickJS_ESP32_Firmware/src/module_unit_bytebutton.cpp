#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_BYTEBUTTON_ENABLE_

#include <Wire.h>
#include "quickjs.h"
#include "module_unit_bytebutton.h"
#include "unit_byte.hpp"

static UnitByte unitbyte;
static const uint8_t buttonId = 0x47;

static JSValue unit_bytebutton_begin(JSContext *ctx, JSValueConst jsThis,
                                      int argc, JSValueConst *argv)
{
  if( !unitbyte.begin(&Wire, buttonId) )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_setFlashWriteBack(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  unitbyte.setFlashWriteBack();

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_getSwitchStatus(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint8_t status = unitbyte.getSwitchStatus();

  return JS_NewUint32(ctx, status);
}

static JSValue unit_bytebutton_getLEDBrightness(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint8_t brightness = unitbyte.getLEDBrightness();

  return JS_NewUint32(ctx, brightness);
}

static JSValue unit_bytebutton_setLEDBrightness(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t brightness;
  JS_ToUint32(ctx, &brightness, argv[0]);

  for( int i = 0 ; i < 9 ; i++ )
    unitbyte.setLEDBrightness(i, (uint8_t)brightness);

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_getLEDShowMode(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint8_t mode = unitbyte.getLEDShowMode();

  return JS_NewUint32(ctx, mode);
}

static JSValue unit_bytebutton_setLEDShowMode(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t mode;
  JS_ToUint32(ctx, &mode, argv[0]);

  byte_led_t ledMode = (byte_led_t)mode;
  unitbyte.setLEDShowMode(ledMode);

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_setSwitchRGB(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t num, onColor, offColor;
  JS_ToUint32(ctx, &num, argv[0]);
  JS_ToUint32(ctx, &onColor, argv[1]);
  JS_ToUint32(ctx, &offColor, argv[2]);

  unitbyte.setSwitchOnRGB888(num, onColor);
  unitbyte.setSwitchOffRGB888(num, offColor);

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_getSwitchRGB(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);

  uint32_t onColor = unitbyte.getSwitchOnRGB888(num);
  uint32_t offColor = unitbyte.getSwitchOffRGB888(num);

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "onColor", JS_NewUint32(ctx, onColor));
  JS_SetPropertyStr(ctx, obj, "offColor", JS_NewUint32(ctx, offColor));

  return obj;
}

static JSValue unit_bytebutton_setRGB(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t num, color;
  JS_ToUint32(ctx, &num, argv[0]);
  JS_ToUint32(ctx, &color, argv[1]);

  unitbyte.setRGB888(num, color);

  return JS_UNDEFINED;
}

static JSValue unit_bytebutton_getRGB(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);

  uint32_t color = unitbyte.getRGB888(num);

  return JS_NewUint32(ctx, color);
}

static const JSCFunctionListEntry unit_bytebutton_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_bytebutton_begin}
        }},
    JSCFunctionListEntry{
        "setFlashWriteBack", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_bytebutton_setFlashWriteBack}
        }},
    JSCFunctionListEntry{
        "getSwitchStatus", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_bytebutton_getSwitchStatus}
        }},
    JSCFunctionListEntry{
        "getLEDBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_bytebutton_getLEDBrightness}
        }},
    JSCFunctionListEntry{
        "setLEDBrightness", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_bytebutton_setLEDBrightness}
        }},
    JSCFunctionListEntry{
        "getLEDShowMode", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_bytebutton_getLEDShowMode}
        }},
    JSCFunctionListEntry{
        "setLEDShowMode", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_bytebutton_setLEDShowMode}
        }},
    JSCFunctionListEntry{
        "getSwitchRGB", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_bytebutton_getSwitchRGB}
        }},
    JSCFunctionListEntry{
        "setSwitchRGB", 0, JS_DEF_CFUNC, 0, {
          func : {3, JS_CFUNC_generic, unit_bytebutton_setSwitchRGB}
        }},
    JSCFunctionListEntry{
        "setRGB", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, unit_bytebutton_setRGB}
        }},
    JSCFunctionListEntry{
        "getRGB", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_bytebutton_getRGB}
        }},
    JSCFunctionListEntry{
        "LED_USER_DEFINED", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BYTE_LED_USER_DEFINED
        }},
    JSCFunctionListEntry{
        "LED_MODE_DEFAULT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : BYTE_LED_MODE_DEFAULT
        }},
};

JSModuleDef *addModule_unit_bytebutton(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitByteButton", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_bytebutton_funcs,
                            sizeof(unit_bytebutton_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_bytebutton_funcs,
        sizeof(unit_bytebutton_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

JsModuleEntry unit_bytebutton_module = {
  NULL,
  addModule_unit_bytebutton,
  NULL,
  NULL
};

#endif