#include <Arduino.h>
#include "main_config.h"

#ifdef _CAMERA_ENABLE_

#include "quickjs.h"
#include "module_type.h"
#include "module_utils.h"

#include "esp_camera.h"

enum camera_pins_type {
  CAMERA_MODEL_WROVER_KIT = 0,  // Has PSRAM
  CAMERA_MODEL_ESP_EYE, // Has PSRAM
  CAMERA_MODEL_M5STACK_PSRAM, // Has PSRAM
  CAMERA_MODEL_M5STACK_V2_PSRAM,  // M5Camera version B Has PSRAM
  CAMERA_MODEL_M5STACK_WIDE, // Has PSRAM
  CAMERA_MODEL_M5STACK_ESP32CAM,  // No PSRAM
  CAMERA_MODEL_AI_THINKER,  // Has PSRAM
  CAMERA_MODEL_TTGO_T_JOURNAL,  // No PSRAM
  CAMERA_MODEL_TTGO_CAMERA, // Has PSRAM
  CAMERA_MODEL_NUM
};

enum camera_pins_index{
  PWDN_GPIO_INDEX = 0,
  RESET_GPIO_INDEX,
  XCLK_GPIO_INDEX,
  SIOD_GPIO_INDEX,
  SIOC_GPIO_INDEX,
  Y9_GPIO_INDEX,
  Y8_GPIO_INDEX,
  Y7_GPIO_INDEX,
  Y6_GPIO_INDEX,
  Y5_GPIO_INDEX,
  Y4_GPIO_INDEX,
  Y3_GPIO_INDEX,
  Y2_GPIO_INDEX,
  VSYNC_GPIO_INDEX,
  HREF_GPIO_INDEX,
  PCLK_GPIO_INDEX
};

static const int8_t camera_pins[][16] = {
  { -1, -1, 21, 26, 27, 35, 34, 39, 36, 19, 18, 5, 4, 25, 23, 22 },
  { -1, -1, 4, 18, 23, 36, 37, 38, 39, 35, 14, 13, 34, 5, 27, 25 },
  { -1, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 32, 22, 26, 21 },
  { -1, 15, 27, 22, 23, 19, 36, 18, 39, 5, 34, 35, 32, 25, 26, 21 },
  { -1, 15, 27, 22, 23, 19, 36, 18, 39, 5, 34, 35, 32, 25, 26, 21 },
  { -1, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 17, 22, 26, 21 },
  { 32, -1, 0, 26, 27, 35, 34, 39, 36, 21, 19, 18, 5, 25, 23, 22 },
  { 0, 15, 27, 25, 23, 19, 36, 18, 39, 5, 34, 35, 17, 22, 26, 21 },
  { -1, -1, 4, 18, 23, 36, 37, 38, 39, 35, 26, 13, 34, 5, 27, 25 },
};

// typedef enum {
//     FRAMESIZE_96X96,    // 96x96
//     FRAMESIZE_QQVGA,    // 160x120
//     FRAMESIZE_QCIF,     // 176x144
//     FRAMESIZE_HQVGA,    // 240x176
//     FRAMESIZE_240X240,  // 240x240
//     FRAMESIZE_QVGA,     // 320x240
//     FRAMESIZE_CIF,      // 400x296
//     FRAMESIZE_HVGA,     // 480x320
//     FRAMESIZE_VGA,      // 640x480
//     FRAMESIZE_SVGA,     // 800x600
//     FRAMESIZE_XGA,      // 1024x768
//     FRAMESIZE_HD,       // 1280x720
//     FRAMESIZE_SXGA,     // 1280x1024
//     FRAMESIZE_UXGA,     // 1600x1200
//     // 3MP Sensors
//     FRAMESIZE_FHD,      // 1920x1080
//     FRAMESIZE_P_HD,     //  720x1280
//     FRAMESIZE_P_3MP,    //  864x1536
//     FRAMESIZE_QXGA,     // 2048x1536
//     // 5MP Sensors
//     FRAMESIZE_QHD,      // 2560x1440
//     FRAMESIZE_WQXGA,    // 2560x1600
//     FRAMESIZE_P_FHD,    // 1080x1920
//     FRAMESIZE_QSXGA,    // 2560x1920
//     FRAMESIZE_INVALID
// } framesize_t;

static bool isInitialized = false;

static long camera_initialize(uint8_t type, uint8_t framesize);
static long camera_dispose(void);
static long camera_get_capture(uint8_t **pp_image, size_t *p_image_size);

static JSValue esp32_camera_start(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( isInitialized )
    return JS_EXCEPTION;

  uint32_t type = CAMERA_MODEL_WROVER_KIT;
  if( argc >= 1 )
    JS_ToUint32(ctx, &type, argv[0]);
  if( type >= CAMERA_MODEL_NUM )
    return JS_EXCEPTION;

  uint32_t framesize = FRAMESIZE_QVGA;
  if( argc >= 2 )
    JS_ToUint32(ctx, &framesize, argv[1]);

  long ret = camera_initialize(type, framesize);
  if( ret != 0 ){
    ret = camera_initialize(type, framesize);
    if( ret != 0 )
      return JS_EXCEPTION;
  }
  isInitialized = true;

  return JS_UNDEFINED;
}

static JSValue esp32_camera_stop(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  camera_dispose();
  isInitialized = false;

  return JS_UNDEFINED;
}

static JSValue esp32_camera_getPicture(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;

  uint8_t *p_image;
  size_t image_size;
  long ret = camera_get_capture(&p_image, &image_size);
  if( ret != 0 )
    return JS_EXCEPTION;

  JSValue value = JS_EXCEPTION;
  value = JS_NewArrayBufferCopy(ctx, p_image, image_size);
  free(p_image);

  return value;
}

static JSValue esp32_camera_setParameter(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  if( !isInitialized )
    return JS_EXCEPTION;
  if( argc < 1 )
    return JS_EXCEPTION;

  sensor_t * s = esp_camera_sensor_get();
  JSValue v;
  v = JS_GetPropertyStr(ctx, argv[0], "framesize");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_framesize(s, (framesize_t)val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "quality"); /* 10 - 63 */
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_quality(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "brightness"); /* -2 - 2 */
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_brightness(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "contrast"); /* -2 - 2 */
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_contrast(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "saturation"); /* -2 - 2 */
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_saturation(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "special_effect");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_special_effect(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "awb");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_whitebal(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "awb_gain");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_awb_gain(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "wb_mode");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_wb_mode(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "aec");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_exposure_ctrl(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "aec2");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_aec2(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "ae_level"); /* -2 - 2 */
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_ae_level(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "agc");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_gain_ctrl(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "agc_gain");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_agc_gain(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "gainceiling");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_gainceiling(s, (gainceiling_t)val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "bpc");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_bpc(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "wpc");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_wpc(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "raw_gma");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_raw_gma(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "lenc");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_lenc(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "hmirror");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_hmirror(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "vflip");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_vflip(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "dcw");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_dcw(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "colorbar");
  if( v != JS_UNDEFINED ){
    int val = JS_ToBool(ctx, val);
    JS_FreeValue(ctx, v);
    s->set_colorbar(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "sharpness");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_sharpness(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "denoise");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_denoise(s, val);
  }
  v = JS_GetPropertyStr(ctx, argv[0], "aec_value");
  if( v != JS_UNDEFINED ){
    int32_t val;
    JS_ToInt32(ctx, &val, v);
    JS_FreeValue(ctx, v);
    s->set_aec_value(s, val);
  }

  return JS_UNDEFINED;
}

static JSValue esp32_camera_getParameter(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  JSValue params = JS_NewObject(ctx);
  sensor_t * s = esp_camera_sensor_get();
  JS_SetPropertyStr(ctx, params, "framesize", JS_NewInt32(ctx, s->status.framesize));
  JS_SetPropertyStr(ctx, params, "quality", JS_NewInt32(ctx, s->status.quality));
  JS_SetPropertyStr(ctx, params, "brightness", JS_NewInt32(ctx, s->status.brightness));
  JS_SetPropertyStr(ctx, params, "contrast", JS_NewInt32(ctx, s->status.contrast));
  JS_SetPropertyStr(ctx, params, "saturation", JS_NewInt32(ctx, s->status.saturation));
  JS_SetPropertyStr(ctx, params, "special_effect", JS_NewInt32(ctx, s->status.special_effect));
  JS_SetPropertyStr(ctx, params, "awb_gain", JS_NewBool(ctx, s->status.awb_gain));
  JS_SetPropertyStr(ctx, params, "wb_mode", JS_NewInt32(ctx, s->status.wb_mode));
  JS_SetPropertyStr(ctx, params, "aec2", JS_NewBool(ctx, s->status.aec2));
  JS_SetPropertyStr(ctx, params, "ae_level", JS_NewInt32(ctx, s->status.ae_level));
  JS_SetPropertyStr(ctx, params, "agc_gain", JS_NewInt32(ctx, s->status.agc_gain));
  JS_SetPropertyStr(ctx, params, "gainceiling", JS_NewInt32(ctx, s->status.gainceiling));
  JS_SetPropertyStr(ctx, params, "bpc", JS_NewBool(ctx, s->status.bpc));
  JS_SetPropertyStr(ctx, params, "wpc", JS_NewBool(ctx, s->status.wpc));
  JS_SetPropertyStr(ctx, params, "raw_gma", JS_NewBool(ctx, s->status.raw_gma));
  JS_SetPropertyStr(ctx, params, "lenc", JS_NewBool(ctx, s->status.lenc));
  JS_SetPropertyStr(ctx, params, "hmirror", JS_NewBool(ctx, s->status.hmirror));
  JS_SetPropertyStr(ctx, params, "vflip", JS_NewBool(ctx, s->status.vflip));
  JS_SetPropertyStr(ctx, params, "dcw", JS_NewBool(ctx, s->status.dcw));

  JS_SetPropertyStr(ctx, params, "colorbar", JS_NewBool(ctx, s->status.colorbar));
  JS_SetPropertyStr(ctx, params, "sharpness", JS_NewInt32(ctx, s->status.sharpness));
  JS_SetPropertyStr(ctx, params, "denoise", JS_NewInt32(ctx, s->status.denoise));
  JS_SetPropertyStr(ctx, params, "aec_value", JS_NewInt32(ctx, s->status.aec_value));

  JS_SetPropertyStr(ctx, params, "awb", JS_NewBool(ctx, s->status.awb));
  JS_SetPropertyStr(ctx, params, "aec", JS_NewBool(ctx, s->status.aec));
  JS_SetPropertyStr(ctx, params, "agc", JS_NewBool(ctx, s->status.agc));

  JSValue info = JS_NewObject(ctx);
  camera_sensor_info_t * s_info = esp_camera_sensor_get_info(&s->id);
  JS_SetPropertyStr(ctx, info, "pid", JS_NewInt32(ctx, s_info->pid));
  JS_SetPropertyStr(ctx, info, "model", JS_NewInt32(ctx, s_info->model));
  JS_SetPropertyStr(ctx, info, "name", JS_NewString(ctx, s_info->name));
  JS_SetPropertyStr(ctx, info, "support_jpeg", JS_NewBool(ctx, s_info->support_jpeg));
  JS_SetPropertyStr(ctx, info, "max_framesize", JS_NewInt32(ctx, s_info->max_size));

  JSValue value = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, value, "info", info);
  JS_SetPropertyStr(ctx, value, "params", params);

  return value;
}

static const JSCFunctionListEntry camera_funcs[] = {
    JSCFunctionListEntry{"start", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_camera_start}
                         }},
    JSCFunctionListEntry{"stop", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_camera_stop}
                         }},
    JSCFunctionListEntry{"getPicture", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_camera_getPicture}
                         }},
    JSCFunctionListEntry{"setParameter", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_camera_setParameter}
                         }},
    JSCFunctionListEntry{"getParameter", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_camera_getParameter}
                         }},
    JSCFunctionListEntry{
        "MODEL_WROVER_KIT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_WROVER_KIT
        }},
    JSCFunctionListEntry{
        "MODEL_ESP_EYE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_ESP_EYE
        }},
    JSCFunctionListEntry{
        "MODEL_M5STACK_PSRAM", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_M5STACK_PSRAM
        }},
    JSCFunctionListEntry{
        "MODEL_M5STACK_V2_PSRAM", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_M5STACK_V2_PSRAM
        }},
    JSCFunctionListEntry{
        "MODEL_M5STACK_WIDE", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_M5STACK_WIDE
        }},
    JSCFunctionListEntry{
        "MODEL_M5STACK_ESP32CAM", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_M5STACK_ESP32CAM
        }},
    JSCFunctionListEntry{
        "MODEL_AI_THINKER", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_AI_THINKER
        }},
    JSCFunctionListEntry{
        "MODEL_TTGO_T_JOURNAL", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_TTGO_T_JOURNAL
        }},
    JSCFunctionListEntry{
        "MODEL_TTGO_CAMERA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_MODEL_TTGO_CAMERA
        }},
    JSCFunctionListEntry{ // 96x96
        "FRAMESIZE_96X96", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_96X96
        }},
    JSCFunctionListEntry{ // 160x120
        "FRAMESIZE_QQVGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QQVGA
        }},
    JSCFunctionListEntry{ // 176x144
        "FRAMESIZE_QCIF", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QCIF
        }},
    JSCFunctionListEntry{ // 240x176
        "FRAMESIZE_HQVGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_HQVGA
        }},
    JSCFunctionListEntry{ // 240x240
        "FRAMESIZE_240X240", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_240X240
        }},
    JSCFunctionListEntry{ // 320x240
        "FRAMESIZE_QVGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QVGA
        }},
    JSCFunctionListEntry{ // 400x296
        "FRAMESIZE_CIF", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_CIF
        }},
    JSCFunctionListEntry{ // 480x320
        "FRAMESIZE_HVGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_HVGA
        }},
    JSCFunctionListEntry{ // 640x480
        "FRAMESIZE_VGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_VGA
        }},
    JSCFunctionListEntry{ // 800x600
        "FRAMESIZE_SVGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_SVGA
        }},
    JSCFunctionListEntry{ // 1024x768
        "FRAMESIZE_XGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_XGA
        }},
    JSCFunctionListEntry{ // 1280x720
        "FRAMESIZE_HD", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_HD
        }},
    JSCFunctionListEntry{ // 1280x1024
        "FRAMESIZE_SXGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_SXGA
        }},
    JSCFunctionListEntry{ // 1600x1200
        "FRAMESIZE_UXGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_UXGA
        }},
    JSCFunctionListEntry{ // 1920x1080
        "FRAMESIZE_FHD", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_FHD
        }},
    JSCFunctionListEntry{ //  720x1280
        "FRAMESIZE_P_HD", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_P_HD
        }},
    JSCFunctionListEntry{ //  864x1536
        "FRAMESIZE_P_3MP", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_P_3MP
        }},
    JSCFunctionListEntry{ // 2048x1536
        "FRAMESIZE_QXGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QXGA
        }},
    JSCFunctionListEntry{ // 2560x1440
        "FRAMESIZE_QHD", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QHD
        }},
    JSCFunctionListEntry{ // 2560x1600
        "FRAMESIZE_WQXGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_WQXGA
        }},
    JSCFunctionListEntry{ // 1080x1920
        "FRAMESIZE_P_FHD", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_P_FHD
        }},
    JSCFunctionListEntry{ // 2560x1920
        "FRAMESIZE_QSXGA", 0, JS_DEF_PROP_INT32, 0, {
          i32 : FRAMESIZE_QSXGA
        }},
    JSCFunctionListEntry{
        "CAMERA_OV7725", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_OV7725
        }},
    JSCFunctionListEntry{
        "CAMERA_OV2640", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_OV2640
        }},
    JSCFunctionListEntry{
        "CAMERA_OV3660", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_OV3660
        }},
    JSCFunctionListEntry{
        "CAMERA_OV5640", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_OV5640
        }},
    JSCFunctionListEntry{
        "CAMERA_OV7670", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_OV7670
        }},
    JSCFunctionListEntry{
        "CAMERA_NT99141", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_NT99141
        }},
    JSCFunctionListEntry{
        "MODEL_GC2145", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_GC2145
        }},
    JSCFunctionListEntry{
        "CAMERA_GC032A", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_GC032A
        }},
    JSCFunctionListEntry{
        "CAMERA_GC0308", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_GC0308
        }},
    JSCFunctionListEntry{
        "CAMERA_BF3005", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_BF3005
        }},
    JSCFunctionListEntry{
        "CAMERA_BF20A6", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_BF20A6
        }},
    JSCFunctionListEntry{
        "CAMERA_SC101IOT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_SC101IOT
        }},
    JSCFunctionListEntry{
        "CAMERA_SC030IOT", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_SC030IOT
        }},
    JSCFunctionListEntry{
        "CAMERA_SC031GS", 0, JS_DEF_PROP_INT32, 0, {
          i32 : CAMERA_SC031GS
        }},
};

JSModuleDef *addModule_camera(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Camera", [](JSContext *ctx, JSModuleDef *m)
                      { return JS_SetModuleExportList(
                            ctx, m, camera_funcs,
                            sizeof(camera_funcs) / sizeof(JSCFunctionListEntry)); });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, camera_funcs,
        sizeof(camera_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

void endModule_camera(void){
  if( isInitialized ){
    camera_dispose();
    isInitialized = false;
  }
}

JsModuleEntry camera_module = {
  NULL,
  addModule_camera,
  NULL,
  endModule_camera
};

static long camera_dispose(void)
{
  esp_camera_deinit();

  return 0;
}

static long camera_initialize(uint8_t type, uint8_t framesize)
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = camera_pins[type][Y2_GPIO_INDEX];
  config.pin_d1 = camera_pins[type][Y3_GPIO_INDEX];
  config.pin_d2 = camera_pins[type][Y4_GPIO_INDEX];
  config.pin_d3 = camera_pins[type][Y5_GPIO_INDEX];
  config.pin_d4 = camera_pins[type][Y6_GPIO_INDEX];
  config.pin_d5 = camera_pins[type][Y7_GPIO_INDEX];
  config.pin_d6 = camera_pins[type][Y8_GPIO_INDEX];
  config.pin_d7 = camera_pins[type][Y9_GPIO_INDEX];
  config.pin_xclk = camera_pins[type][XCLK_GPIO_INDEX];
  config.pin_pclk = camera_pins[type][PCLK_GPIO_INDEX];
  config.pin_vsync = camera_pins[type][VSYNC_GPIO_INDEX];
  config.pin_href = camera_pins[type][HREF_GPIO_INDEX];
  config.pin_sccb_sda = camera_pins[type][SIOD_GPIO_INDEX];
  config.pin_sccb_scl = camera_pins[type][SIOC_GPIO_INDEX];
  config.pin_pwdn = camera_pins[type][PWDN_GPIO_INDEX];
  config.pin_reset = camera_pins[type][RESET_GPIO_INDEX];

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.

//  config.frame_size = FRAMESIZE_QVGA;
  config.frame_size = (framesize_t)framesize;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return -1;
  }

  return 0;
}

static long camera_get_capture(uint8_t **pp_image, size_t *p_image_size)
{
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if( fb )
    esp_camera_fb_return(fb);
  fb = esp_camera_fb_get();
  if (!fb){
      Serial.println("Camera capture failed");
      return -1;
  }

  if (fb->format != PIXFORMAT_JPEG){
    Serial.println("Unknown format");
    esp_camera_fb_return(fb);
    return -1;
  }
  
  *pp_image = (uint8_t*)malloc(fb->len);
  if( *pp_image == NULL ){
    Serial.println("Out of memory");
    esp_camera_fb_return(fb);
    return -1;
  }
  memmove(*pp_image, fb->buf, fb->len);
  *p_image_size = fb->len;
  esp_camera_fb_return(fb);

  return 0;
}

#endif
