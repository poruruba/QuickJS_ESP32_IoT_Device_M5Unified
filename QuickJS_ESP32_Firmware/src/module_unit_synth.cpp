#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_SYNTH_ENABLE_

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_unit_synth.h"
#include "MML_Synth.h"
#include <M5UnitSynth.h>

#define SYNTH_DEFAULT_VOLUME 100.0
#define SYNTH_DEFAULT_CHANNEL 0
#define SYNTH_DEFAULT_PROGRAM 0

static M5UnitSynth synth;
static MML_Synth mml;
static TimerHandle_t mmlTimer;
static bool timerRunning = false;
static uint8_t g_ledc_ch = 0;
static uint32_t g_period = 10;
static uint8_t g_resolution = 10;
static bool g_repeat = false;
static float g_volume = SYNTH_DEFAULT_VOLUME;
static char *gp_mml_text = NULL;

static void startIntervalTimer(uint32_t period);
static void stopIntervalTimer(void);

static void func_tone(uint8_t pitch, uint16_t vol) {
  synth.setNoteOn(SYNTH_DEFAULT_CHANNEL, pitch, vol / 15 * 127 * (g_volume / 100.0));
}

static void func_notone() {
}

static void func_instrument(uint8_t value) {
  Serial.printf("set instrument: %d\n", value);
  synth.setInstrument(0, SYNTH_DEFAULT_CHANNEL, value);
}

static void debug(uint8_t c) {
  Serial.write(c);
}

static JSValue unit_synth_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t rx_pin, tx_pin;
  JS_ToUint32(ctx, &rx_pin, argv[0]);
  JS_ToUint32(ctx, &tx_pin, argv[1]);
  uint32_t period;
  JS_ToUint32(ctx, &period, argv[2]);

  g_period = period;

  synth.begin(&Serial2, UNIT_SYNTH_BAUD, tx_pin, rx_pin);

  return JS_UNDEFINED;
}

static JSValue unit_synth_play(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *text = JS_ToCString(ctx, argv[0]);
  if( text == NULL )
    return JS_EXCEPTION;
  
  bool repeat = false;
  if( argc > 1 )
    repeat = JS_ToBool(ctx, argv[1]);

  stopIntervalTimer();

  gp_mml_text = strdup(text);
  JS_FreeCString(ctx, text);

  mml.init(nullptr, func_tone, func_notone, func_instrument, debug);
  mml.setText(gp_mml_text);
  g_repeat = repeat;
  mml.playBGM();

  startIntervalTimer(g_period);

  return JS_UNDEFINED;
}

static JSValue unit_synth_stop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  stopIntervalTimer();

  return JS_UNDEFINED;
}

static JSValue unit_synth_setVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  double volume;
  JS_ToFloat64(ctx, &volume, argv[0]);
  g_volume = volume;

  return JS_UNDEFINED;
}

static JSValue unit_synth_getVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewFloat64(ctx, g_volume);
}

static JSValue unit_synth_setTempo(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t tempo;
  JS_ToUint32(ctx, &tempo, argv[0]);
  mml.tempo(tempo);

  return JS_UNDEFINED;
}

static JSValue unit_synth_isPlaying(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, mml.isBGMPlay());
}

static JSValue unit_synth_setNoteOn(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t channel, pitch, velocity;
  JS_ToUint32(ctx, &channel, argv[0]);
  JS_ToUint32(ctx, &pitch, argv[1]);
  JS_ToUint32(ctx, &velocity, argv[2]);

  synth.setNoteOn(channel, pitch, velocity);

  return JS_UNDEFINED;
}

static JSValue unit_synth_setNoteOff(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t channel, pitch, velocity;
  JS_ToUint32(ctx, &channel, argv[0]);
  JS_ToUint32(ctx, &pitch, argv[1]);

  synth.setNoteOff(channel, pitch, 0);

  return JS_UNDEFINED;
}

static JSValue unit_synth_setInstrument(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t bank;
  JS_ToUint32(ctx, &bank, argv[0]);

  synth.setInstrument(0, SYNTH_DEFAULT_CHANNEL, bank);

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry unit_synth_funcs[] = {
    JSCFunctionListEntry{"begin", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic, unit_synth_begin}
                        }},
    JSCFunctionListEntry{"play", 0, JS_DEF_CFUNC, 0, {
                          func : {2, JS_CFUNC_generic, unit_synth_play}
                        }},
    JSCFunctionListEntry{"stop", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, unit_synth_stop}
                        }},
    JSCFunctionListEntry{"setVolume", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic, unit_synth_setVolume}
                        }},
    JSCFunctionListEntry{"getVolume", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, unit_synth_getVolume}
                        }},
    JSCFunctionListEntry{"setTempo", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic, unit_synth_setTempo}
                        }},
    JSCFunctionListEntry{"isPlaying", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, unit_synth_isPlaying}
                        }},
    JSCFunctionListEntry{"setInstrument", 0, JS_DEF_CFUNC, 0, {
                          func : {1, JS_CFUNC_generic, unit_synth_setInstrument}
                        }},
    JSCFunctionListEntry{"setNoteOn", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic, unit_synth_setNoteOn}
                        }},
    JSCFunctionListEntry{"setNoteOff", 0, JS_DEF_CFUNC, 0, {
                          func : {2, JS_CFUNC_generic, unit_synth_setNoteOff}
                        }},
};

JSModuleDef *addModule_synth(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitSynth", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_synth_funcs,
                            sizeof(unit_synth_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_synth_funcs,
        sizeof(unit_synth_funcs) / sizeof(JSCFunctionListEntry));
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
      if( g_repeat ){
        mml.playBGM();
        startIntervalTimer(g_period);
      }
//      Serial.println("startIntervalTimer end");
    }
  }
}

static void stopIntervalTimer(void){
  if( timerRunning ){
    g_repeat = false;
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

long initialize_synth(void)
{
  mmlTimer = xTimerCreate(
    "mmlTimer",
    pdMS_TO_TICKS(g_period),
    pdTRUE,
    nullptr,
    mmlTimerCallback
  );

  return 0;
}

void endModule_synth(void){
  stopIntervalTimer();
  g_volume = SYNTH_DEFAULT_VOLUME;
  synth.setInstrument(0, SYNTH_DEFAULT_CHANNEL, SYNTH_DEFAULT_PROGRAM);
}

JsModuleEntry unit_synth_module = {
  initialize_synth,
  addModule_synth,
  NULL,
  endModule_synth
};

#endif
