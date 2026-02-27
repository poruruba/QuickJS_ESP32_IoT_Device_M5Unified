#include <Arduino.h>
#include "main_config.h"

#ifdef _MML_ENABLE_

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_mml.h"
#include "MML.h"

#define MML_DEFAULT_VOLUME 50.0

static MML mml;
static TimerHandle_t mmlTimer;
static bool timerRunning = false;
static uint8_t g_ledc_ch = 0;
static uint32_t g_period = 10;
static uint8_t g_resolution = 10;
static float g_volume = MML_DEFAULT_VOLUME;
static char *gp_mml_text = NULL;

static void startIntervalTimer(uint32_t period);
static void stopIntervalTimer(void);

static void ledcTone(uint32_t freq) {
  if (freq == 0) {
    ledcWrite(g_ledc_ch, 0);
    return;
  }
  ledcWriteTone(g_ledc_ch, freq);
  ledcWrite(g_ledc_ch, ((1ul << g_resolution) - 1) * g_volume / 100);
}

static void func_tone(uint16_t freq, uint16_t tm, uint16_t vol) {
  ledcTone(freq);
}

static void func_notone() {
  ledcTone(0);
}

static JSValue mml_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t channel;
  JS_ToUint32(ctx, &channel, argv[0]);
  uint32_t period;
  JS_ToUint32(ctx, &period, argv[1]);
  uint32_t resolution;
  JS_ToUint32(ctx, &resolution, argv[2]);

  g_ledc_ch = (uint8_t)channel;
  g_period = period;
  g_resolution = resolution;

  return JS_UNDEFINED;
}

static JSValue mml_play(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *text = JS_ToCString(ctx, argv[0]);
  if( text == NULL )
    return JS_EXCEPTION;

  stopIntervalTimer();

  gp_mml_text = strdup(text);
  JS_FreeCString(ctx, text);

  mml.setText(gp_mml_text);
  mml.playBGM();

  startIntervalTimer(g_period);

  return JS_UNDEFINED;
}

static JSValue mml_stop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  stopIntervalTimer();

  return JS_UNDEFINED;
}

static JSValue mml_setVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  double volume;
  JS_ToFloat64(ctx, &volume, argv[0]);
  g_volume = volume;

  return JS_UNDEFINED;
}

static JSValue mml_getVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewFloat64(ctx, g_volume);
}

static JSValue mml_isPlaying(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, mml.isBGMPlay());
}

static const JSCFunctionListEntry mml_funcs[] = {
    JSCFunctionListEntry{"begin", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic, mml_begin}
                        }},
    JSCFunctionListEntry{"play", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic, mml_play}
                        }},
    JSCFunctionListEntry{"stop", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic, mml_stop}
                        }},
    JSCFunctionListEntry{"setVolume", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic, mml_setVolume}
                        }},
    JSCFunctionListEntry{"getVolume", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, mml_getVolume}
                        }},
    JSCFunctionListEntry{"isPlaying", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, mml_isPlaying}
                        }},
};

JSModuleDef *addModule_mml(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "Mml", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, mml_funcs,
                            sizeof(mml_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, mml_funcs,
        sizeof(mml_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

static void mmlTimerCallback(TimerHandle_t xTimer) {
  if( timerRunning ){
    if (mml.isBGMPlay()) {
      if (mml.available()) 
        mml.playTick();
    }else{
      xTimerStop(mmlTimer, 0);
      timerRunning = false;
//      Serial.println("startIntervalTimer end");
    }
  }
}

static void stopIntervalTimer(void){
  if( timerRunning ){
    mml.stop();
    xTimerStop(mmlTimer, 0);
    timerRunning = false;
    if( gp_mml_text != NULL ){
      free(gp_mml_text);
      gp_mml_text = NULL;
    }
  }
}

static void startIntervalTimer(uint32_t period){
  if( timerRunning ){
    xTimerStop(mmlTimer, 0);
    timerRunning = false;
  }

//  Serial.println("startIntervalTimer start");
  xTimerChangePeriod(mmlTimer, pdMS_TO_TICKS(period), 0);
  xTimerStart(mmlTimer, 0);
  timerRunning = true;
}

long initialize_mml(void)
{
  mml.init(nullptr, func_tone, func_notone, nullptr);
  mmlTimer = xTimerCreate(
    "mmlTimer",
    pdMS_TO_TICKS(g_period),
    pdTRUE,
    nullptr,
    mmlTimerCallback
  );

  return 0;
}

void endModule_mml(void){
  stopIntervalTimer();
  g_volume = MML_DEFAULT_VOLUME;
}

JsModuleEntry mml_module = {
  initialize_mml,
  addModule_mml,
  NULL,
  endModule_mml
};

#endif
