#include <Arduino.h>
#include "main_config.h"
#include "quickjs.h"
#include "module_type.h"
#include "module_input.h"
#include "quickjs_esp32.h"

#include "MyButton.h"

static MyButton *g_BtnX = NULL;
static MyButton *g_BtnY = NULL;
static MyButton *g_BtnZ = NULL;

bool module_input_checkButtonState(uint8_t type, uint8_t value0, uint32_t value1)
{
  bool result = false;

  switch(type){
    case FUNC_TYPE_IS_PRESSED: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.isPressed(); break;
        case INPUT_BUTTON_B: result = M5.BtnB.isPressed(); break;
        case INPUT_BUTTON_C: result = M5.BtnC.isPressed(); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.isPressed(); break;
        case INPUT_BUTTON_X: result = g_BtnX->isPressed(); break;
        case INPUT_BUTTON_Y: result = g_BtnY->isPressed(); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->isPressed(); break;
      }
      break;
    }
    case FUNC_TYPE_IS_RELEASED: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.isReleased(); break;
        case INPUT_BUTTON_B: result = M5.BtnB.isReleased(); break;
        case INPUT_BUTTON_C: result = M5.BtnC.isReleased(); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.isReleased(); break;
        case INPUT_BUTTON_X: result = g_BtnX->isReleased(); break;
        case INPUT_BUTTON_Y: result = g_BtnY->isReleased(); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->isReleased(); break;
      }
      break;
    }
    case FUNC_TYPE_WAS_PRESSED: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.wasPressed(); break;
        case INPUT_BUTTON_B: result = M5.BtnB.wasPressed(); break;
        case INPUT_BUTTON_C: result = M5.BtnC.wasPressed(); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.wasPressed(); break;
        case INPUT_BUTTON_X: result = g_BtnX->wasPressed(); break;
        case INPUT_BUTTON_Y: result = g_BtnY->wasPressed(); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->wasPressed(); break;
      }
      break;
    }
    case FUNC_TYPE_WAS_RELEASED: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.wasReleased(); break;
        case INPUT_BUTTON_B: result = M5.BtnB.wasReleased(); break;
        case INPUT_BUTTON_C: result = M5.BtnC.wasReleased(); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.wasReleased(); break;
        case INPUT_BUTTON_X: result = g_BtnX->wasReleased(); break;
        case INPUT_BUTTON_Y: result = g_BtnY->wasReleased(); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->wasReleased(); break;
      }
      break;
    }
    case FUNC_TYPE_PRESSED_FOR: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.pressedFor(value1); break;
        case INPUT_BUTTON_B: result = M5.BtnB.pressedFor(value1); break;
        case INPUT_BUTTON_C: result = M5.BtnC.pressedFor(value1); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.pressedFor(value1); break;
        case INPUT_BUTTON_X: result = g_BtnX->pressedFor(value1); break;
        case INPUT_BUTTON_Y: result = g_BtnY->pressedFor(value1); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->pressedFor(value1); break;
      }
      break;
    }
    case FUNC_TYPE_RELEASED_FOR: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.releasedFor(value1); break;
        case INPUT_BUTTON_B: result = M5.BtnB.releasedFor(value1); break;
        case INPUT_BUTTON_C: result = M5.BtnC.releasedFor(value1); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.releasedFor(value1); break;
        case INPUT_BUTTON_X: result = g_BtnX->releasedFor(value1); break;
        case INPUT_BUTTON_Y: result = g_BtnY->releasedFor(value1); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->releasedFor(value1); break;
      }
      break;
    }
    case FUNC_TYPE_WAS_RELEASE_FOR: {
      switch(value0){
        case INPUT_BUTTON_A: result = M5.BtnA.wasReleaseFor(value1); break;
        case INPUT_BUTTON_B: result = M5.BtnB.wasReleaseFor(value1); break;
        case INPUT_BUTTON_C: result = M5.BtnC.wasReleaseFor(value1); break;
        case INPUT_BUTTON_PWR: result = M5.BtnPWR.wasReleaseFor(value1); break;
        case INPUT_BUTTON_X: result = g_BtnX->wasReleaseFor(value1); break;
        case INPUT_BUTTON_Y: result = g_BtnY->wasReleaseFor(value1); break;
        case INPUT_BUTTON_Z: result = g_BtnZ->wasReleaseFor(value1); break;
      }
      break;
    }
  }

  return result;
}

static JSValue input_checkButtonState(JSContext *ctx, JSValueConst jsThis, int argc,
                                JSValueConst *argv, int magic)
{
  uint32_t value0, value1 = 0;
  JS_ToUint32(ctx, &value0, argv[0]);
  if (magic == FUNC_TYPE_PRESSED_FOR || magic == FUNC_TYPE_RELEASED_FOR || magic == FUNC_TYPE_WAS_RELEASE_FOR)
    JS_ToUint32(ctx, &value1, argv[1]);
  
  if( (value0 == INPUT_BUTTON_X && g_BtnX == NULL) ||
      (value0 == INPUT_BUTTON_Y && g_BtnY == NULL) ||
      (value0 == INPUT_BUTTON_Z && g_BtnZ == NULL) )
    return JS_EXCEPTION;

  bool result = module_input_checkButtonState(magic, value0, value1);
  return JS_NewBool(ctx, result);
}

static JSValue input_onButtonWasPressed(JSContext *ctx, JSValueConst jsThis, int argc,
                            JSValueConst *argv) {
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  ESP32QuickJS *qjs = (ESP32QuickJS *)JS_GetContextOpaque(ctx);
  qjs->setBtnFunc(JS_DupValue(ctx, argv[1]), value);

  return JS_UNDEFINED;
}

static JSValue input_isTouched(JSContext *ctx, JSValueConst jsThis,
                                     int argc, JSValueConst *argv)
{
  if( M5.Touch.getCount() > 0 ){
    m5::touch_detail_t pos = M5.Touch.getDetail();
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, pos.x));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, pos.y));
    return obj;
  }else{
    return JS_NULL;
  }
}

static JSValue input_openCustomButton(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t btnId;
  JS_ToUint32(ctx, &btnId, argv[0]);
  uint32_t pinNo;
  JS_ToUint32(ctx, &pinNo, argv[1]);
  bool invert;
  invert = JS_ToBool(ctx, argv[2]);

  if( btnId == INPUT_BUTTON_X ){
    if( g_BtnX != NULL )
      delete g_BtnX;
    g_BtnX = new MyButton(pinNo, invert, 10);
  }else if( btnId == INPUT_BUTTON_Y ){
    if( g_BtnX != NULL )
      delete g_BtnY;
    g_BtnY = new MyButton(pinNo, invert, 10);
  }else if( btnId == INPUT_BUTTON_Z ){
    if( g_BtnZ != NULL )
      delete g_BtnZ;
    g_BtnZ = new MyButton(pinNo, invert, 10);
  }else{
    return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static JSValue input_closeCustomButton(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t btnId;
  JS_ToUint32(ctx, &btnId, argv[0]);

  if( btnId == INPUT_BUTTON_Y ){
    if( g_BtnX != NULL ){
      delete g_BtnX;
      g_BtnX = NULL;
    }
  }else if( btnId == INPUT_BUTTON_Y ){
    if( g_BtnY != NULL ){
      delete g_BtnY;
      g_BtnX = NULL;
    }
  }else if( btnId == INPUT_BUTTON_Z ){
    if( g_BtnZ != NULL ){
      delete g_BtnZ;
      g_BtnZ = NULL;
    }
  }else{
    return JS_EXCEPTION;
  }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry input_funcs[] = {
    JSCFunctionListEntry{"isPressed", 0, JS_DEF_CFUNC, FUNC_TYPE_IS_PRESSED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"isReleased", 0, JS_DEF_CFUNC, FUNC_TYPE_IS_RELEASED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasPressed", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_PRESSED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasReleased", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_RELEASED, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"pressedFor", 0, JS_DEF_CFUNC, FUNC_TYPE_PRESSED_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"releasedFor", 0, JS_DEF_CFUNC, FUNC_TYPE_RELEASED_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"wasReleaseFor", 0, JS_DEF_CFUNC, FUNC_TYPE_WAS_RELEASE_FOR, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : input_checkButtonState}}
                         }},
    JSCFunctionListEntry{"onButtonWasPressed", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, input_onButtonWasPressed}
                         }},
    JSCFunctionListEntry{"openCustomButton", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, input_openCustomButton}
                         }},
    JSCFunctionListEntry{"closeCustomButton", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, input_closeCustomButton}
                         }},
    JSCFunctionListEntry{
        "isTouched", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, input_isTouched}
        }},
    JSCFunctionListEntry{
        "BUTTON_A", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_A
        }},
    JSCFunctionListEntry{
        "BUTTON_B", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_B
        }},
    JSCFunctionListEntry{
        "BUTTON_C", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_C
        }},
    JSCFunctionListEntry{
        "BUTTON_PWR", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_PWR
        }},
    JSCFunctionListEntry{
        "BUTTON_X", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_X
        }},
    JSCFunctionListEntry{
        "BUTTON_Y", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_Y
        }},
    JSCFunctionListEntry{
        "BUTTON_Z", 0, JS_DEF_PROP_INT32, 0, {
          i32 : INPUT_BUTTON_Z
        }},
};

JSModuleDef *addModule_input(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Input", [](JSContext *ctx, JSModuleDef *m) {
          return JS_SetModuleExportList(
              ctx, m, input_funcs,
              sizeof(input_funcs) / sizeof(JSCFunctionListEntry));
        });
  if (mod) {
    JS_AddModuleExportList(
        ctx, mod, input_funcs,
        sizeof(input_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_input(void)
{
  if( g_BtnX != NULL ){
    delete g_BtnX;
    g_BtnX = NULL;
  }
  if( g_BtnY != NULL ){
    delete g_BtnY;
    g_BtnX = NULL;
  }
  if( g_BtnZ != NULL ){
    delete g_BtnZ;
    g_BtnZ = NULL;
  }
}

void loopModule_input(void){
  if( g_BtnX != NULL ){
    g_BtnX->read();
  }
  if( g_BtnY != NULL ){
    g_BtnY->read();
  }
  if( g_BtnZ != NULL ){
    g_BtnZ->read();
  }
}

JsModuleEntry input_module = {
  NULL,
  addModule_input,
  loopModule_input,
  endModule_input
};