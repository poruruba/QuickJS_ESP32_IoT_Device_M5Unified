#include <Arduino.h>
#include "main_config.h"

#ifdef _UNIT_AUDIOPLAYER_ENABLE_

#include "quickjs.h"
#include "module_unit_audioplayer.h"
#include "unit_audioplayer.hpp"

static AudioPlayerUnit audioplayer;
static bool isBegan = false;

static JSValue unit_audioplayer_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t tx, rx;
  JS_ToUint32(ctx, &tx, argv[0]);
  JS_ToUint32(ctx, &rx, argv[1]);

  if( isBegan ){
    audioplayer.endAudio();
    isBegan = false;
  }
  bool ret = audioplayer.begin(&Serial1, tx, rx);
  if( ret )
    isBegan = true;

  return JS_NewBool(ctx, ret);
}

static JSValue unit_audioplayer_end(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isBegan ){
    audioplayer.endAudio();
    isBegan = false;
  }

  return JS_UNDEFINED;
}

static JSValue unit_audioplayer_setVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t volume;
  JS_ToUint32(ctx, &volume, argv[0]);

  audioplayer.setVolume(volume);

  return JS_UNDEFINED;
}

static JSValue unit_audioplayer_setPlayMode(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t mode;
  JS_ToUint32(ctx, &mode, argv[0]);

  audioplayer.setPlayMode((play_mode_t)mode);

  return JS_UNDEFINED;
}

static JSValue unit_audioplayer_playAudio(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  audioplayer.playAudio();

  return JS_UNDEFINED;
}

static JSValue unit_audioplayer_selectAudioNum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t num;
  JS_ToUint32(ctx, &num, argv[0]);

  uint16_t ret = audioplayer.selectAudioNum(num);

  return JS_NewUint32(ctx, ret);
}

static JSValue unit_audioplayer_checkPlayStatus(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  play_status_t status = audioplayer.checkPlayStatus();

  return JS_NewUint32(ctx, status);
}

static JSValue unit_audioplayer_getVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t volume = audioplayer.getVolume();

  return JS_NewUint32(ctx, volume);
}

static JSValue unit_audioplayer_getCurrentAudioNumber(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t number = audioplayer.getCurrentAudioNumber();

  return JS_NewUint32(ctx, number);
}

static JSValue unit_audioplayer_previousAudio(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t number = audioplayer.previousAudio();

  return JS_NewUint32(ctx, number);
}

static JSValue unit_audioplayer_pauseAudio(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  play_status_t status = audioplayer.pauseAudio();

  return JS_NewUint32(ctx, status);
}

static JSValue unit_audioplayer_nextAudio(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint16_t number = audioplayer.nextAudio();

  return JS_NewUint32(ctx, number);
}

static const JSCFunctionListEntry unit_audioplayer_funcs[] = {
    JSCFunctionListEntry{
        "begin", 0, JS_DEF_CFUNC, 0, {
          func : {2, JS_CFUNC_generic, unit_audioplayer_begin}
        }},
    JSCFunctionListEntry{
        "end", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_end}
        }},
    JSCFunctionListEntry{
        "setVolume", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_audioplayer_setVolume}
        }},
    JSCFunctionListEntry{
        "getVolume", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_getVolume}
        }},
    JSCFunctionListEntry{
        "setPlayMode", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_audioplayer_setPlayMode}
        }},
    JSCFunctionListEntry{
        "playAudio", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_playAudio}
        }},
    JSCFunctionListEntry{
        "checkPlayStatus", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_checkPlayStatus}
        }},
    JSCFunctionListEntry{
        "selectAudioNumber", 0, JS_DEF_CFUNC, 0, {
          func : {1, JS_CFUNC_generic, unit_audioplayer_selectAudioNum}
        }},
    JSCFunctionListEntry{
        "getCurrentAudioNumber", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_getCurrentAudioNumber}
        }},
    JSCFunctionListEntry{
        "previousAudio", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_previousAudio}
        }},
    JSCFunctionListEntry{
        "pauseAudio", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_pauseAudio}
        }},
    JSCFunctionListEntry{
        "nextAudio", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic, unit_audioplayer_nextAudio}
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_ALL_LOOP", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_ALL_LOOP
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_SINGLE_LOOP", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_SINGLE_LOOP
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_FOLDER_LOOP", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_FOLDER_LOOP
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_RANDOM", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_RANDOM
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_SINGLE_STOP", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_SINGLE_STOP
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_ALL_ONCE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_ALL_ONCE
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_FOLDER_ONCE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_FOLDER_ONCE
        }},
    JSCFunctionListEntry{
        "PLAY_MODE_ERROR", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_MODE_ERROR
        }},
    JSCFunctionListEntry{
        "PLAY_STATUS_STOPPED", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_STATUS_STOPPED
        }},
    JSCFunctionListEntry{
        "PLAY_STATUS_PLAYING", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_STATUS_PLAYING
        }},
    JSCFunctionListEntry{
        "PLAY_STATUS_PAUSED", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_STATUS_PAUSED
        }},
    JSCFunctionListEntry{
        "PLAY_STATUS_ERROR", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_PLAYER_STATUS_ERROR
        }},
};

JSModuleDef *addModule_unit_audioplayer(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;
  mod = JS_NewCModule(ctx, "UnitAudioPlayer", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, unit_audioplayer_funcs,
                            sizeof(unit_audioplayer_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, unit_audioplayer_funcs,
        sizeof(unit_audioplayer_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void loopModule_unit_audioplayer(void)
{
  if( isBegan )
    audioplayer.update();
}

void endModule_unit_audioplayer(void)
{
  if( isBegan ){
    audioplayer.endAudio();
    isBegan = false;
  }
}

JsModuleEntry unit_audioplayer_module = {
  NULL,
  addModule_unit_audioplayer,
  loopModule_unit_audioplayer,
  endModule_unit_audioplayer
};

#endif