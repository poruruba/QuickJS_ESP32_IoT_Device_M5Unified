#include <Arduino.h>
#include "main_config.h"

#ifdef _LCD_ENABLE_

#ifdef _SD_ENABLE_
#include <SD.h>
#endif

#include "quickjs.h"
#include "module_lcd.h"
#include "module_utils.h"
#include "module_type.h"

#define NUM_OF_SPRITE   5
static LGFX_Sprite* sprites[NUM_OF_SPRITE];

#define FONT_COLOR TFT_WHITE

static JSValue esp32_lcd_setRotation(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  M5.Display.setRotation(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setBrigthness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  M5.Display.setBrightness(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextColor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value0, value1;
  if( argc == 1 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    M5.Display.setTextColor(value0);
  }else if( argc == 2 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    JS_ToUint32(ctx, &value1, argv[1]);
    M5.Display.setTextColor(value0, value1);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextSize(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  double valueX, valueY;
  if( argc == 1 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    M5.Display.setTextSize(valueX);
  }else if( argc == 2 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    JS_ToFloat64(ctx, &valueY, argv[1]);
    M5.Display.setTextSize(valueX, valueY);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextDatum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  M5.Display.setTextDatum(value);
  return JS_UNDEFINED;
}

#ifdef _SD_ENABLE_
static JSValue esp32_lcd_draw_image_file(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *fpath = JS_ToCString(ctx, argv[0]);
  if( fpath == NULL )
    return JS_EXCEPTION;
  File file = SD.open(fpath, FILE_READ);
  if( !file ){
    JS_FreeCString(ctx, fpath);
    return JS_EXCEPTION;
  }
  uint8_t image_buffer[4];
  int size = file.read(image_buffer, sizeof(image_buffer));
  file.close();
  if( size < sizeof(image_buffer) ){
    JS_FreeCString(ctx, fpath);
    return JS_EXCEPTION;
  }

  int32_t x = 0, y = 0;
  if( argc >= 2 )
  JS_ToInt32(ctx, &x, argv[1]);
  if( argc >= 3 )
  JS_ToInt32(ctx, &y, argv[2]);
  bool ret = false;
  if( image_buffer[0] == 0xff && image_buffer[1] == 0xd8 ){
    ret = M5.Display.drawJpgFile(SD, fpath, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    ret = M5.Display.drawPngFile(SD, fpath, x, y);
  }
  JS_FreeCString(ctx, fpath);

  return JS_NewBool(ctx, ret);
}
#endif

static JSValue esp32_lcd_draw_image_url(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *url = JS_ToCString(ctx, argv[0]);
  if( url == NULL )
    return JS_EXCEPTION;
  
  uint32_t size;
  uint8_t *image_buffer = http_get_binary2(url, &size);
  JS_FreeCString(ctx, url);
  if( image_buffer == NULL )
    return JS_EXCEPTION;

  int32_t x = 0, y = 0;
  if( argc >= 2 )
    JS_ToInt32(ctx, &x, argv[1]);
  if( argc >= 3 )
    JS_ToInt32(ctx, &y, argv[2]);

  bool ret = false;
  if( image_buffer[0] == 0xff && image_buffer[1] == 0xd8 ){
    ret = M5.Display.drawJpg(image_buffer, size, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    ret = M5.Display.drawPng(image_buffer, size, x, y);
  }

  return JS_NewBool(ctx, ret);
}

static JSValue esp32_lcd_print(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  long ret;
  const char *text = JS_ToCString(ctx, argv[0]);
  if (magic == 0){
    ret = M5.Display.print(text);
  }else
  if (magic == 1){
    ret = M5.Display.println(text);
  }else{
    JS_FreeCString(ctx, text);
    return JS_EXCEPTION;
  }

  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, ret);
}

long module_lcd_setFont(uint16_t size)
{
  switch (size){
//    case 8 : M5.Display.setFont(&fonts::lgfxJapanGothic_8); break;
    case 12 : M5.Display.setFont(&fonts::lgfxJapanGothic_12); break;
//    case 16 : M5.Display.setFont(&fonts::lgfxJapanGothic_16); break;
//    case 20 : M5.Display.setFont(&fonts::lgfxJapanGothic_20); break;
//    case 24 : M5.Display.setFont(&fonts::lgfxJapanGothic_24); break;
//    case 28 : M5.Display.setFont(&fonts::lgfxJapanGothic_28); break;
    case 32 : M5.Display.setFont(&fonts::lgfxJapanGothic_32); break;
//    case 36 : M5.Display.setFont(&fonts::lgfxJapanGothic_36); break;
//    case 40 : M5.Display.setFont(&fonts::lgfxJapanGothic_40); break;
  }

  return 0;
}

static JSValue esp32_lcd_setFont(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  module_lcd_setFont(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  M5.Display.setCursor(value0, value1);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_getCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{

  JSValue obj = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, M5.Display.getCursorX()));
  JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, M5.Display.getCursorY()));
  return obj;
}

static JSValue esp32_lcd_textWidth(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *text = JS_ToCString(ctx, argv[0]);
  int32_t width = M5.Display.textWidth(text);
  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, width);
}

static JSValue esp32_lcd_drawPixel(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  uint32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToUint32(ctx, &value2, argv[2]);
  M5.Display.drawPixel(value0, value1, value2);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawLine(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  int32_t value3;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  JS_ToInt32(ctx, &value3, argv[3]);
  if(argc >= 5){
    uint32_t value4;
    JS_ToUint32(ctx, &value4, argv[4]);
    M5.Display.drawLine(value0, value1, value2, value3, value4);
  }else{
    M5.Display.drawLine(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillCircle(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  if(argc >= 4){
    uint32_t value3;
    JS_ToUint32(ctx, &value3, argv[3]);
    if( magic == FUNC_TYPE_DRAWCIRCLE )
      M5.Display.drawCircle(value0, value1, value2, value3);
    else if( magic == FUNC_TYPE_FILLCIRCLE)
      M5.Display.fillCircle(value0, value1, value2, value3);
  }else{
    if( magic == FUNC_TYPE_DRAWCIRCLE )
      M5.Display.drawCircle(value0, value1, value2);
    else if( magic == FUNC_TYPE_FILLCIRCLE)
      M5.Display.fillCircle(value0, value1, value2);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  int32_t value3;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  JS_ToInt32(ctx, &value3, argv[3]);
  if(argc >= 5){
    uint32_t value4;
    JS_ToUint32(ctx, &value4, argv[4]);
    if( magic == FUNC_TYPE_DRAWRECT )
      M5.Display.drawRect(value0, value1, value2, value3, value4);
    else if( magic == FUNC_TYPE_FILLRECT)
      M5.Display.fillRect(value0, value1, value2, value3, value4);
  }else{
    if( magic == FUNC_TYPE_DRAWRECT )
      M5.Display.drawRect(value0, value1, value2, value3);
    else if( magic == FUNC_TYPE_FILLRECT)
      M5.Display.fillRect(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillRoundRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  int32_t value0;
  int32_t value1;
  int32_t value2;
  int32_t value3;
    int32_t value4;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  JS_ToInt32(ctx, &value3, argv[3]);
    JS_ToInt32(ctx, &value4, argv[4]);
  if(argc >= 6){
    uint32_t value5;
    JS_ToUint32(ctx, &value5, argv[5]);
    if( magic == FUNC_TYPE_DRAWROUNDRECT )
      M5.Display.drawRoundRect(value0, value1, value2, value3, value4, value5);
    else if( magic == FUNC_TYPE_FILLROUNDRECT)
      M5.Display.fillRoundRect(value0, value1, value2, value3, value4, value5);
  }else{
    if( magic == FUNC_TYPE_DRAWROUNDRECT )
      M5.Display.drawRoundRect(value0, value1, value2, value3, value4);
    else if( magic == FUNC_TYPE_FILLROUNDRECT)
      M5.Display.fillRoundRect(value0, value1, value2, value3, value4);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_getMetric(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  int32_t value = 0;

  switch(magic){
    case FUNC_TYPE_WIDTH: value = M5.Display.width(); break;
    case FUNC_TYPE_HEIGHT: value = M5.Display.height(); break;
    case FUNC_TYPE_DEPTH: value = M5.Display.getColorDepth(); break;
    case FUNC_TYPE_FONTHEIGHT: value = M5.Display.fontHeight(); break;
  }
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_fillScreen(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  M5.Display.fillScreen(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_createSpriteFromBmp(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint8_t *p_buffer;
  uint8_t unit_size;
  uint32_t unit_num;
  JSValue vbuffer = getTypedArrayBuffer(ctx, argv[0], (void**)&p_buffer, &unit_size, &unit_num);
  if( JS_IsNull(vbuffer) )
    return JS_EXCEPTION;
  if( unit_size != 1 ){
    JS_FreeValue(ctx, vbuffer);
    return JS_EXCEPTION;
  }

  for( int i = 0 ; i < NUM_OF_SPRITE ; i++ ){
    if(sprites[i] == NULL ){
      sprites[i] = new LGFX_Sprite(&M5.Display);
      sprites[i]->createFromBmp(p_buffer, unit_num);
      JS_FreeValue(ctx, vbuffer);
      return JS_NewUint32(ctx, i);
    }
  }
  JS_FreeValue(ctx, vbuffer);

  return JS_EXCEPTION;
}

#ifdef _SD_ENABLE_
static JSValue esp32_lcd_createSpriteFromBmpFile(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  const char *path = JS_ToCString(ctx, argv[0]);
  if( path == NULL )
    return JS_EXCEPTION;

  for( int i = 0 ; i < NUM_OF_SPRITE ; i++ ){
    if(sprites[i] == NULL ){
      sprites[i] = new LGFX_Sprite(&M5.Display);
      sprites[i]->createFromBmpFile(SD, path);
      JS_FreeCString(ctx, path);
      return JS_NewUint32(ctx, i);
    }
  }
  JS_FreeCString(ctx, path);

  return JS_EXCEPTION;
}
#endif

static JSValue esp32_lcd_freeSprite(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t id;
  JS_ToUint32(ctx, &id, argv[0]);

  if( id >= NUM_OF_SPRITE )
    return JS_EXCEPTION;

  if( sprites[id] == NULL )
    return JS_UNDEFINED;
  
  delete sprites[id];
  sprites[id] = NULL;

  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setPivot(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t id;
  JS_ToUint32(ctx, &id, argv[0]);

  if( id >= NUM_OF_SPRITE )
    return JS_EXCEPTION;

  if( sprites[id] == NULL )
    return JS_EXCEPTION;
  
  double x, y;
  JS_ToFloat64(ctx, &x, argv[1]);
  JS_ToFloat64(ctx, &y, argv[2]);

  sprites[id]->setPivot(x, y);

  return JS_UNDEFINED;
}

static JSValue esp32_lcd_pushSprite(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t id;
  JS_ToUint32(ctx, &id, argv[0]);
  
  if( id >= NUM_OF_SPRITE )
    return JS_EXCEPTION;

  if( sprites[id] == NULL )
    return JS_EXCEPTION;
  
  int32_t x, y;
  JS_ToInt32(ctx, &x, argv[1]);
  JS_ToInt32(ctx, &y, argv[2]);

  if( argc >= 4 ){
    uint32_t transp;
    JS_ToUint32(ctx, &transp, argv[3]);
    sprites[id]->pushSprite(x, y, transp);
  }else{
    sprites[id]->pushSprite(x, y);
  }

  return JS_UNDEFINED;
}

static JSValue esp32_lcd_pushRotateZoom(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  uint32_t id;
  JS_ToUint32(ctx, &id, argv[0]);
  
  if( id >= NUM_OF_SPRITE )
    return JS_EXCEPTION;

  if( sprites[id] == NULL )
    return JS_EXCEPTION;
  
  double dst_x, dst_y, angle, zoom_x, zoom_y;
  JS_ToFloat64(ctx, &dst_x, argv[1]);
  JS_ToFloat64(ctx, &dst_y, argv[2]);
  JS_ToFloat64(ctx, &angle, argv[3]);
  JS_ToFloat64(ctx, &zoom_x, argv[4]);
  JS_ToFloat64(ctx, &zoom_y, argv[5]);

  if( argc >= 7 ){
    uint32_t transp;
    JS_ToUint32(ctx, &transp, argv[6]);
    sprites[id]->pushRotateZoom(dst_x, dst_y, angle, zoom_x, zoom_y, transp);
  }else{
    sprites[id]->pushRotateZoom(dst_x, dst_y, angle, zoom_x, zoom_y);
  }

  return JS_UNDEFINED;
}

static const JSCFunctionListEntry lcd_funcs[] = {
    JSCFunctionListEntry{"setRotation", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setRotation}
                         }},
    JSCFunctionListEntry{"setBrightness", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setBrigthness}
                         }},
    JSCFunctionListEntry{"setFont", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setFont}
                         }},
    JSCFunctionListEntry{"setTextColor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setTextColor}
                         }},
    JSCFunctionListEntry{"setTextSize", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setTextSize}
                         }},
    JSCFunctionListEntry{"setTextDatum", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_setTextDatum}
                         }},
    JSCFunctionListEntry{"drawPixel", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_drawPixel}
                         }},
    JSCFunctionListEntry{"drawLine", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic, esp32_lcd_drawLine}
                         }},
    JSCFunctionListEntry{"drawCircle", 0, JS_DEF_CFUNC, FUNC_TYPE_DRAWCIRCLE, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillCircle}}
                         }},
    JSCFunctionListEntry{"fillCircle", 0, JS_DEF_CFUNC, FUNC_TYPE_FILLCIRCLE, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillCircle}}
                         }},
    JSCFunctionListEntry{"drawRect", 0, JS_DEF_CFUNC, FUNC_TYPE_DRAWRECT, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRect}}
                         }},
    JSCFunctionListEntry{"fillRect", 0, JS_DEF_CFUNC, FUNC_TYPE_FILLRECT, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRect}}
                         }},
    JSCFunctionListEntry{"drawRoundRect", 0, JS_DEF_CFUNC, FUNC_TYPE_DRAWROUNDRECT, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRoundRect}}
                         }},
    JSCFunctionListEntry{"fillRoundRect", 0, JS_DEF_CFUNC, FUNC_TYPE_FILLROUNDRECT, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRoundRect}}
                         }},
    JSCFunctionListEntry{"setCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic, esp32_lcd_setCursor}
                         }},
    JSCFunctionListEntry{"getCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_lcd_getCursor}
                         }},
    JSCFunctionListEntry{"textWidth", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_textWidth}
                         }},
    JSCFunctionListEntry{"print", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"println", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"fillScreen", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_fillScreen}
                         }},
#ifdef _SD_ENABLE_
    JSCFunctionListEntry{"drawImageFile", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_draw_image_file}
                         }},
#endif
    JSCFunctionListEntry{"drawImageUrl", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_draw_image_url}
                         }},
    JSCFunctionListEntry{"width", 0, JS_DEF_CFUNC, FUNC_TYPE_WIDTH, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"height", 0, JS_DEF_CFUNC, FUNC_TYPE_HEIGHT, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"getColorDepth", 0, JS_DEF_CFUNC, FUNC_TYPE_DEPTH, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
    JSCFunctionListEntry{"fontHeight", 0, JS_DEF_CFUNC, FUNC_TYPE_FONTHEIGHT, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getMetric}}
                         }},
#ifdef _SD_ENABLE_
    JSCFunctionListEntry{"createSpriteFromBmpFile", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_createSpriteFromBmpFile}
                         }},
#endif
    JSCFunctionListEntry{"createSpriteFromBmp", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_createSpriteFromBmp}
                         }},
    JSCFunctionListEntry{"freeSprites", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic, esp32_lcd_freeSprite}
                         }},
    JSCFunctionListEntry{"setPivot", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic, esp32_lcd_setPivot}
                         }},
    JSCFunctionListEntry{"pushSprite", 0, JS_DEF_CFUNC, 0, {
                           func : {4, JS_CFUNC_generic, esp32_lcd_pushSprite}
                         }},
    JSCFunctionListEntry{"pushRotateZoom", 0, JS_DEF_CFUNC, 0, {
                           func : {7, JS_CFUNC_generic, esp32_lcd_pushRotateZoom}
                         }},
    JSCFunctionListEntry{
        "top_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_left
        }},
    JSCFunctionListEntry{
        "top_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_center
        }},
    JSCFunctionListEntry{
        "top_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::top_right
        }},
    JSCFunctionListEntry{
        "middle_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_left
        }},
    JSCFunctionListEntry{
        "middle_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_center
        }},
    JSCFunctionListEntry{
        "middle_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::middle_right
        }},
    JSCFunctionListEntry{
        "bottom_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_left
        }},
    JSCFunctionListEntry{
        "bottom_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_center
        }},
    JSCFunctionListEntry{
        "bottom_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::bottom_right
        }},
    JSCFunctionListEntry{
        "baseline_left", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_left
        }},
    JSCFunctionListEntry{
        "baseline_center", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_center
        }},
    JSCFunctionListEntry{
        "baseline_right", 0, JS_DEF_PROP_INT32, 0, {
          i32 : lgfx::baseline_right
        }},
};

JSModuleDef *addModule_lcd(JSContext *ctx, JSValue global)
{
  JSModuleDef *mod;

  mod = JS_NewCModule(ctx, "Lcd", [](JSContext *ctx, JSModuleDef *m){
        return JS_SetModuleExportList(
                            ctx, m, lcd_funcs,
                            sizeof(lcd_funcs) / sizeof(JSCFunctionListEntry));
      });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, lcd_funcs,
        sizeof(lcd_funcs) / sizeof(JSCFunctionListEntry));
  }

  return mod;
}

long initialize_lcd(void)
{
  M5.Display.setFont(&fonts::lgfxJapanGothic_16);
  M5.Display.setColorDepth(16);
  M5.Display.setBrightness(128);
  M5.Display.setRotation(1);
  M5.Display.setCursor(0, 0);
  M5.Display.setTextColor(FONT_COLOR);

  return 0;
}

void endModule_lcd(void)
{
  for( int i = 0 ; i < NUM_OF_SPRITE ; i++ ){
    if( sprites[i] != NULL ){
      delete sprites[i];
      sprites[i] = NULL;
    }
  }
}

JsModuleEntry lcd_module = {
  initialize_lcd,
  addModule_lcd,
  NULL,
  endModule_lcd
};

#endif
