#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_SYNTH_ENABLE_

#include "quickjs.h"
#include "quickjs_esp32.h"
#include "module_unit_synth.h"
#include "MML_Synth.h"
#include <M5UnitSynth.h>

#define SYNTH_DEFAULT_VOLUME 100.0
#define SYNTH_DEFAULT_PROGRAM 0
#define SYNTH_DEFAULT_BANK 0

#define NUM_OF_SYNCH  3

static M5UnitSynth synth;
static MML_Synth mml[NUM_OF_SYNCH];
static TimerHandle_t mmlTimer;
static bool timerRunning = false;
static uint32_t g_period = 0;
static uint8_t g_resolution = 10;
static bool g_repeat = false;
static float g_volume = SYNTH_DEFAULT_VOLUME;
static char *gp_mml_text[NUM_OF_SYNCH];

static void startIntervalTimer(uint32_t period);
static void stopIntervalTimer(void);

static void func_tone(uint8_t idx,uint8_t pitch, uint16_t vol) {
  synth.setNoteOn(idx, pitch, vol / 15.0 * 127 * (g_volume / 100.0));
}

static void func_notone(uint8_t idx) {
}

static void func_instrument(uint8_t idx, uint8_t value) {
  synth.setInstrument(SYNTH_DEFAULT_BANK, idx, value);
}

static void debug(uint8_t idx, uint8_t c) {
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
  stopIntervalTimer();

  if( JS_IsArray(ctx, argv[0]) ){
    JSValue value = JS_GetPropertyStr(ctx, argv[0], "length");
    uint32_t length;
    JS_ToUint32(ctx, &length, value);
    JS_FreeValue(ctx, value);
    if( length > NUM_OF_SYNCH )
      return JS_EXCEPTION;

    for( int i = 0 ; i < length ; i++ ){
      JSValue element = JS_GetPropertyUint32(ctx, argv[0], i);
      const char *text = JS_ToCString(ctx, element);
      if( text == NULL ){
        for( int j = 0 ; j < i ; j++ ){
          free(gp_mml_text[j]);
          gp_mml_text[j] = NULL;
        }
        JS_FreeValue(ctx, element);
        return JS_EXCEPTION;
      }
      gp_mml_text[i] = strdup(text);
      JS_FreeCString(ctx, text);
      JS_FreeValue(ctx, element);
    }
  }else if( JS_IsString(argv[0]) ){
    const char *text = JS_ToCString(ctx, argv[0]);
    if( text == NULL )
      return JS_EXCEPTION;
    gp_mml_text[0] = strdup(text);
    JS_FreeCString(ctx, text);
  }else{
    return JS_EXCEPTION;
  }
  
  bool repeat = false;
  if( argc > 1 )
    repeat = JS_ToBool(ctx, argv[1]);

  for( int i = 0 ; i < NUM_OF_SYNCH ; i++ ){
    if( gp_mml_text[i] == NULL )
      break;
    mml[i].init(nullptr, func_tone, func_notone, func_instrument, /* nullptr */ debug);
    mml[i].setText(gp_mml_text[i]);
  }

  g_repeat = repeat;

  for( int i = 0 ; i < NUM_OF_SYNCH ; i++ ){
    if( gp_mml_text[i] == NULL )
      break;
    mml[i].playBGM();
  }

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

static JSValue unit_synth_reset(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  for( int i = 0 ; i < NUM_OF_SYNCH ; i++ ){
    mml[i].reset();
  }

  return JS_NewBool(ctx, mml[0].isBGMPlay());
}

static JSValue unit_synth_isPlaying(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewBool(ctx, mml[0].isBGMPlay());
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
  uint32_t idx;
  JS_ToUint32(ctx, &idx, argv[0]);
  if( idx >= NUM_OF_SYNCH )
    return JS_EXCEPTION;
  uint32_t val;
  JS_ToUint32(ctx, &val, argv[1]);

  synth.setInstrument(SYNTH_DEFAULT_BANK, idx, val);

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
    JSCFunctionListEntry{"reset", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, unit_synth_reset}
                        }},
    JSCFunctionListEntry{"isPlaying", 0, JS_DEF_CFUNC, 0, {
                          func : {0, JS_CFUNC_generic, unit_synth_isPlaying}
                        }},
    JSCFunctionListEntry{"setInstrument", 0, JS_DEF_CFUNC, 0, {
                          func : {2, JS_CFUNC_generic, unit_synth_setInstrument}
                        }},
    JSCFunctionListEntry{"setNoteOn", 0, JS_DEF_CFUNC, 0, {
                          func : {3, JS_CFUNC_generic, unit_synth_setNoteOn}
                        }},
    JSCFunctionListEntry{"setNoteOff", 0, JS_DEF_CFUNC, 0, {
                          func : {2, JS_CFUNC_generic, unit_synth_setNoteOff}
                        }},
    JSCFunctionListEntry{
        "CATEGORY_PIANO_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 0
        }},
    JSCFunctionListEntry{
        "CATEGORY_CHROMATIC_PERCUSSION_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 8
        }},
    JSCFunctionListEntry{
        "CATEGORY_ORGAN_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 16
        }},
    JSCFunctionListEntry{
        "CATEGORY_GUITAR_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 24
        }},
    JSCFunctionListEntry{
        "CATEGORY_BASS_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 32
        }},
    JSCFunctionListEntry{
        "CATEGORY_STRINGS_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 40
        }},
    JSCFunctionListEntry{
        "CATEGORY_ENSEMBLE_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 48
        }},
    JSCFunctionListEntry{
        "CATEGORY_BRASS_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 56
        }},
    JSCFunctionListEntry{
        "CATEGORY_REED_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 64
        }},
    JSCFunctionListEntry{
        "CATEGORY_PIPE_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 72
        }},
    JSCFunctionListEntry{
        "CATEGORY_SYNTH_LEAD_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 80
        }},
    JSCFunctionListEntry{
        "CATEGORY_SYNTH_PAD_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 88
        }},
    JSCFunctionListEntry{
        "CATEGORY_SYNTH_EFFECTS_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 96
        }},
    JSCFunctionListEntry{
        "CATEGORY_ETHNIC_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 104
        }},
    JSCFunctionListEntry{
        "CATEGORY_PERCUSSIVE_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 112
        }},
    JSCFunctionListEntry{
        "CATEGORY_SOUND_EFFECTS_00", 0, JS_DEF_PROP_INT32, 0, {
          i32 : 120
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
    if (mml[0].isBGMPlay()) {
      uint32_t msec = millis();
      for( int i = 0 ; i < NUM_OF_SYNCH ; i++ ){
        if( gp_mml_text[i] == NULL )
          break;
        if (mml[i].available(msec))
          mml[i].playTick(msec);
      }
    }else{
      xTimerStop(mmlTimer, 0);
      timerRunning = false;
      if( g_repeat ){
        for( int i = 0 ; i < NUM_OF_SYNCH ; i++ ){
          if( gp_mml_text[i] == NULL )
            break;
          mml[i].playBGM();
        }
        startIntervalTimer(g_period);
      }
//      Serial.println("startIntervalTimer end");
    }
  }
}

static void stopIntervalTimer(void){
  if( timerRunning ){
    g_repeat = false;
    xTimerStop(mmlTimer, 0);
    for( int i = 0 ; i < NUM_OF_SYNCH; i++ ){
      if( gp_mml_text[i] == NULL )
        break;
      free(gp_mml_text[i]);
      gp_mml_text[i] = NULL;
      mml[i].stop();
    }
    timerRunning = false;
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
  for( int i = 0 ; i < NUM_OF_SYNCH; i++ ){
    mml[i] = MML_Synth(i);
    gp_mml_text[i] = NULL;
  }

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
  if( g_period > 0 ){
    for( int i = 0 ; i < NUM_OF_SYNCH; i++ ){
      synth.setInstrument(SYNTH_DEFAULT_BANK, i, SYNTH_DEFAULT_PROGRAM);
      mml[i].reset();
    }
  }
}

JsModuleEntry unit_synth_module = {
  initialize_synth,
  addModule_synth,
  NULL,
  endModule_synth
};

#endif
