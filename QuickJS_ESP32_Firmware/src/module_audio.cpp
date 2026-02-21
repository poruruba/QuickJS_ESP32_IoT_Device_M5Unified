#include <Arduino.h>
#include "main_config.h"

#ifdef _AUDIO_ENABLE_

#include <M5Unified.h>
#include "quickjs.h"
#include "module_type.h"
#include "module_utils.h"

#include "module_audio.h"
#include <AudioOutput.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSD.h>

#define DEFAULT_AUDIO_VOLUME  64
#define AUDIO_URL_BUFFER_SIZE 1024

#define AUDIO_OUTPUT_AUTO           0x00
#define AUDIO_OUTPUT_INTERNAL_DAC   0x01
#define AUDIO_OUTPUT_EXTERNAL_I2S   0x02

class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
      : _m5sound(m5sound), _virtual_ch(virtual_sound_channel){}
    
    bool setBufferSize(uint32_t buf_size){
      for (int i = 0; i < 3; i++) {
        if( _tri_buffer[i] != NULL ){
          free(_tri_buffer[i]);
          _tri_buffer[i] = NULL;
        }
      }
      for (int i = 0; i < 3; i++) {
        _tri_buffer[i] = (int16_t*)malloc(buf_size * sizeof(int16_t));
        if (_tri_buffer[i] == NULL) {
          for (int j = 0; j < i; j++) {
            free(_tri_buffer[j]);
            _tri_buffer[j] = NULL;
          }
          return false;
        }
      }
      tri_buf_size = buf_size;
      return true;
    }

    virtual ~AudioOutputM5Speaker(void) {
      for (int i = 0; i < 3; i++) {
        if( _tri_buffer[i] != NULL )
          free(_tri_buffer[i]);
      }
    };
    
    virtual bool begin(void) override { return true; }

    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      if (_tri_buffer_index < tri_buf_size)
      {
        _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
        _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
        _tri_buffer_index += 2;

        return true;
      }

      flush();
      return false;
    }

    virtual void flush(void) override
    {
      if (_tri_buffer_index)
      {
        _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
      }
    }

    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_virtual_ch);
      return true;
    }

    const int16_t* getBuffer(void) const { return _tri_buffer[(_tri_index + 2) % 3]; }

    virtual bool SetRate(int hz) override
    {
//      Serial.printf("SetRate called: %d\n", hz);
      hertz = hz;
      return true;
    }

  protected:
    m5::Speaker_Class* _m5sound;
    uint8_t _virtual_ch;
    size_t tri_buf_size = 0;
    int16_t *_tri_buffer[3] = { NULL, NULL, NULL };
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
};

static AudioOutputM5Speaker *out = NULL;
static AudioGeneratorMP3 *mp3 = NULL;
static AudioFileSourceSD *file_sd = NULL;
static AudioFileSourceHTTPStream *file_http = NULL;
static AudioFileSourceBuffer *buff = NULL;
static bool audio_paused = false;
static uint8_t audio_volume = DEFAULT_AUDIO_VOLUME;

static void audio_source_dispose(void){
  if( mp3 != NULL )
    mp3->stop();
  if( buff != NULL )
    buff->close();
  if( file_sd != NULL )
    file_sd->close();
  if( file_http != NULL )
    file_http->close();

  if( mp3 != NULL ){
    delete mp3;
    mp3 = NULL;
  }

  if( buff != NULL ){
    delete buff;
    buff = NULL;
  }
  if( file_sd != NULL ){
    delete file_sd;
    file_sd = NULL;
  }
  if( file_http != NULL ){
    delete file_http;
    file_http = NULL;
  }

  audio_paused = false;
}

static JSValue audio_begin(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  audio_source_dispose();
  if( out != NULL ){
    out->stop();
    delete out;
    out = NULL;
  }

  uint32_t sample_rate = 48000; // recommend
  if( argc > 0 )
    JS_ToUint32(ctx, &sample_rate, argv[0]);
  uint32_t buf_size = 1536; // recommend
  if( argc > 1 )
    JS_ToUint32(ctx, &buf_size, argv[1]);
  int32_t output_mode = AUDIO_OUTPUT_AUTO;
  if( argc > 2 )
    JS_ToInt32(ctx, &output_mode, argv[2]);

  auto spk_cfg = M5.Speaker.config();
  spk_cfg.sample_rate = sample_rate;
    
  if( output_mode == AUDIO_OUTPUT_INTERNAL_DAC ){
    spk_cfg.use_dac = true;
  }else if( output_mode == AUDIO_OUTPUT_EXTERNAL_I2S ){
    spk_cfg.use_dac = false;
  }else if( output_mode == AUDIO_OUTPUT_AUTO ){
  }else{
    return JS_EXCEPTION;
  }

  if( argc > 3 ){
    uint32_t pin;
    JSValue v2;
    v2 = JS_GetPropertyStr(ctx, argv[3], "bck");
    if( v2 != JS_UNDEFINED ){
      JS_ToUint32(ctx, &pin, v2);
      spk_cfg.pin_bck = pin;
      JS_FreeValue(ctx, v2);
    }
    v2 = JS_GetPropertyStr(ctx, argv[3], "lrck");
    if( v2 != JS_UNDEFINED ){
      JS_ToUint32(ctx, &pin, v2);
      spk_cfg.pin_ws = pin;
      JS_FreeValue(ctx, v2);
    }
    v2 = JS_GetPropertyStr(ctx, argv[3], "dout");
    if( v2 != JS_UNDEFINED ){
      JS_ToUint32(ctx, &pin, v2);
      spk_cfg.pin_data_out = pin;
      JS_FreeValue(ctx, v2);
    }
    v2 = JS_GetPropertyStr(ctx, argv[3], "mck");
    if( v2 != JS_UNDEFINED ){
      JS_ToUint32(ctx, &pin, v2);
      spk_cfg.pin_mck = pin;
      JS_FreeValue(ctx, v2);
    }
  }

  if( M5.Speaker.isRunning() )
    M5.Speaker.end();
//  Serial.printf("use_dac=%d bck=%d ws=%d data_out=%d mck=%d buf_size=%d sample=%d\n",
//                  spk_cfg.use_dac, spk_cfg.pin_bck, spk_cfg.pin_ws, spk_cfg.pin_data_out, spk_cfg.pin_mck, buf_size, sample_rate);
  M5.Speaker.config(spk_cfg);
  M5.Speaker.begin();

  out = new AudioOutputM5Speaker(&M5.Speaker);
  if( !out->setBufferSize(buf_size) )
    return JS_EXCEPTION;

  return JS_UNDEFINED;
}

static JSValue audio_update(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( mp3 != NULL ){
    if (mp3->isRunning()) {
      if( !audio_paused ){
        if (!mp3->loop()){
//          Serial.println("mp3 loop stop");
          mp3->stop();
        }
      }
    }
  }

  return JS_UNDEFINED;
}

static JSValue audio_pause(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( mp3 != NULL ){
    if (mp3->isRunning()) {
      if( magic == 0 ){
        M5.Speaker.setVolume(audio_volume);
        audio_paused = false;
      }else{
        M5.Speaker.setVolume(0);
        out->flush();
        audio_paused = true;
      }
    }
  }

  return JS_UNDEFINED;
}

static JSValue audio_playUrl(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( out == NULL )
    return JS_EXCEPTION;

  audio_source_dispose();

  const char *url = JS_ToCString(ctx, argv[0]);
  if( url == NULL )
    return JS_EXCEPTION;

  uint32_t bufsize = AUDIO_URL_BUFFER_SIZE;
  if( argc >= 2 )
    JS_ToUint32(ctx, &bufsize, argv[1]);

  file_http = new AudioFileSourceHTTPStream(url);
  JS_FreeCString(ctx, url);
  buff = new AudioFileSourceBuffer(file_http, bufsize);
  mp3 = new AudioGeneratorMP3();
  bool ret = mp3->begin(buff, out);
  if( !ret )
    audio_source_dispose();
  
  return JS_NewBool(ctx, ret);
}

static JSValue audio_playSd(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( out == NULL )
    return JS_EXCEPTION;

  audio_source_dispose();
    
  const char *path = JS_ToCString(ctx, argv[0]);
  if( path == NULL )
    return JS_EXCEPTION;

  file_sd = new AudioFileSourceSD(path);
  JS_FreeCString(ctx, path);
  if( !file_sd->isOpen() ){
    delete file_sd;
    file_sd = NULL;
    return JS_EXCEPTION;
  }
  mp3 = new AudioGeneratorMP3();
  bool ret = mp3->begin(file_sd, out);
  if( !ret )
    audio_source_dispose();

  return JS_NewBool(ctx, ret);
}

static JSValue audio_tone(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( out == NULL )
    return JS_EXCEPTION;

  audio_source_dispose();

  double frequency;
  JS_ToFloat64(ctx, &frequency, argv[0]);

  uint32_t duration = UINT32_MAX;
  JS_ToUint32(ctx, &duration, argv[1]);

  bool stop_current_sound = true;
  if( argc > 2 )
    stop_current_sound = JS_ToBool(ctx, argv[2]);

  bool ret = M5.Speaker.tone(frequency, duration, 0, stop_current_sound);

  return JS_NewBool(ctx, ret);
}

static JSValue audio_setVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( out == NULL )
    return JS_EXCEPTION;
    
  double volume;
  JS_ToFloat64(ctx, &volume, argv[0]);

  audio_volume = volume / 100.0 * 255.0;
  if( !audio_paused )
    M5.Speaker.setVolume((uint8_t)audio_volume);

  return JS_UNDEFINED;
}

static JSValue audio_getVolume(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewFloat64(ctx, audio_volume / 255.0 * 100);
}

static JSValue audio_isRunning(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( out == NULL || mp3 == NULL )
    return JS_NewBool(ctx, false);

  bool is_running = mp3->isRunning();

  return JS_NewBool(ctx, is_running);
}

static JSValue audio_stop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  audio_source_dispose();

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry audio_funcs[] = {
    JSCFunctionListEntry{"begin", 0, JS_DEF_CFUNC, 0, {
                           func : {4, JS_CFUNC_generic, audio_begin}
                         }},
    JSCFunctionListEntry{"update", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, audio_update}
                         }},
    JSCFunctionListEntry{
        "resume", 0, JS_DEF_CFUNC, 0, {
          func : {0, JS_CFUNC_generic_magic, {generic_magic : audio_pause}}
        }},
    JSCFunctionListEntry{
        "pause", 0, JS_DEF_CFUNC, 1, {
          func : {0, JS_CFUNC_generic_magic, {generic_magic : audio_pause}}
        }},
    JSCFunctionListEntry{"playUrl", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, audio_playUrl}
                         }},
    JSCFunctionListEntry{"playSd", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, audio_playSd}
                         }},
    JSCFunctionListEntry{"setVolume", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, audio_setVolume}
                         }},
    JSCFunctionListEntry{"getVolume", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, audio_getVolume}
                         }},
    JSCFunctionListEntry{"tone", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, audio_tone}
                         }},
    JSCFunctionListEntry{"isRunning", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, audio_isRunning}
                         }},
    JSCFunctionListEntry{"stop", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, audio_stop}
                         }},
    JSCFunctionListEntry{
        "OUTPUT_AUTO", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_OUTPUT_AUTO
        }},
    JSCFunctionListEntry{
        "OUTPUT_INTERNAL_DAC", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_OUTPUT_INTERNAL_DAC
        }},
    JSCFunctionListEntry{
        "OUTPUT_EXTERNAL_I2S", 0, JS_DEF_PROP_INT32, 0, {
          i32 : AUDIO_OUTPUT_EXTERNAL_I2S
        }},
};

JSModuleDef *addModule_audio(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Audio", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, audio_funcs,
                            sizeof(audio_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, audio_funcs,
        sizeof(audio_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_audio(void)
{
  audio_source_dispose();
  if( out != NULL ){
    out->stop();
    delete out;
    out = NULL;
  }
  audio_volume = DEFAULT_AUDIO_VOLUME;
}

JsModuleEntry audio_module = {
  NULL,
  addModule_audio,
  NULL,
  endModule_audio
};

#endif