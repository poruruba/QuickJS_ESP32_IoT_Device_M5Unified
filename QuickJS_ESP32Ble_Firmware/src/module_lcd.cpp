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
#include "module_esp32.h"

#define NUM_OF_SPRITE   5
static LGFX_Sprite* sprites[NUM_OF_SPRITE];

#define FONT_COLOR TFT_WHITE

static JSValue esp32_lcd_setRotation(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  if( magic == 1 )
    M5.Displays(g_external_display).setRotation(value);
  else
    M5.Display.setRotation(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setBrigthness(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  if( magic == 1 )
    M5.Displays(g_external_display).setBrightness(value);
  else
    M5.Display.setBrightness(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextColor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  uint32_t value0, value1;
  if( argc == 1 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    if( magic == 1 )
      M5.Displays(g_external_display).setTextColor(value0);
    else
      M5.Display.setTextColor(value0);
  }else if( argc == 2 ){
    JS_ToUint32(ctx, &value0, argv[0]);
    JS_ToUint32(ctx, &value1, argv[1]);
    if( magic == 1 )
      M5.Displays(g_external_display).setTextColor(value0, value1);
    else
      M5.Display.setTextColor(value0, value1);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextSize(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  double valueX, valueY;
  if( argc == 1 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    if( magic == 1 )
      M5.Displays(g_external_display).setTextSize(valueX);
    else
      M5.Display.setTextSize(valueX);
  }else if( argc == 2 ){
    JS_ToFloat64(ctx, &valueX, argv[0]);
    JS_ToFloat64(ctx, &valueY, argv[1]);
    if( magic == 1 )
      M5.Displays(g_external_display).setTextSize(valueX, valueY);
    else
      M5.Display.setTextSize(valueX, valueY);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setTextDatum(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  if( magic == 1 )
    M5.Displays(g_external_display).setTextDatum(value);
  else
    M5.Display.setTextDatum(value);
  return JS_UNDEFINED;
}

#ifdef _SD_ENABLE_
static JSValue esp32_lcd_draw_image_file(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawJpgFile(SD, fpath, x, y);
    else
      ret = M5.Display.drawJpgFile(SD, fpath, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawPngFile(SD, fpath, x, y);
    else
      ret = M5.Display.drawPngFile(SD, fpath, x, y);
  }
  JS_FreeCString(ctx, fpath);

  return JS_NewBool(ctx, ret);
}
#endif

#ifndef _WIFI_DISABLE_
static JSValue esp32_lcd_draw_image_url(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawJpg(image_buffer, size, x, y);
    else
      ret = M5.Display.drawJpg(image_buffer, size, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawPng(image_buffer, size, x, y);
    else
      ret = M5.Display.drawPng(image_buffer, size, x, y);
  }

  return JS_NewBool(ctx, ret);
}
#endif

static JSValue esp32_lcd_draw_image(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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

  int32_t x = 0, y = 0;
  int32_t maxWidth = 0, maxHeight = 0;
  int32_t offX = 0, offY = 0;
  double scale_x = 1.0f, scale_y = 0.0f;
  int32_t datum = lgfx::top_left;

  if( argc >= 2 )
    JS_ToInt32(ctx, &x, argv[1]);
  if( argc >= 3 )
    JS_ToInt32(ctx, &y, argv[2]);
  if( argc >= 4 )
    JS_ToInt32(ctx, &maxWidth, argv[3]);
  if( argc >= 5 )
    JS_ToInt32(ctx, &maxHeight, argv[4]);
  if( argc >= 6 )
    JS_ToInt32(ctx, &offX, argv[5]);
  if( argc >= 7 )
    JS_ToInt32(ctx, &offY, argv[6]);
  if( argc >= 8 )
    JS_ToFloat64(ctx, &scale_x, argv[7]);
  if( argc >= 9 )
    JS_ToFloat64(ctx, &scale_y, argv[8]);
  if( argc >= 10 )
    JS_ToInt32(ctx, &datum, argv[9]);

  lgfx::datum_t _datum = (lgfx::datum_t)datum;
  bool ret = false;
  if( p_buffer[0] == 0xff && p_buffer[1] == 0xd8 ){
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawJpg(p_buffer, unit_num, x, y, maxWidth, maxHeight, offX, offY, scale_x, scale_y, _datum);
    else
      ret = M5.Display.drawJpg(p_buffer, unit_num, x, y, maxWidth, maxHeight, offX, offY, scale_x, scale_y, _datum);
  }else if (p_buffer[0] == 0x89 && p_buffer[1] == 0x50 && p_buffer[2] == 0x4e && p_buffer[3] == 0x47 ){
    if( magic == 1 )
      ret = M5.Displays(g_external_display).drawPng(p_buffer, unit_num, x, y, maxWidth, maxHeight, offX, offY, scale_x, scale_y, _datum);
    else
      ret = M5.Display.drawPng(p_buffer, unit_num, x, y, maxWidth, maxHeight, offX, offY, scale_x, scale_y, _datum);
  }
  JS_FreeValue(ctx, vbuffer);

  return JS_NewBool(ctx, ret);
}

static JSValue esp32_lcd_print(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  const char *text = JS_ToCString(ctx, argv[0]);
  long ret;
  if( magic == 1 )
    ret = M5.Displays(g_external_display).print(text);
  else
    ret = M5.Display.print(text);
  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, ret);
}

static JSValue esp32_lcd_println(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  const char *text = JS_ToCString(ctx, argv[0]);
  long ret;
  if( magic == 1 )
    ret = M5.Displays(g_external_display).println(text);
  else
    ret = M5.Display.println(text);
  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, ret);
}

long module_lcd_setFont(uint16_t size, int magic)
{
  if( magic == 1 ){
    switch (size){
  //    case 8 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_8); break;
      case 12 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_12); break;
  //    case 16 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_16); break;
  //    case 20 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_20); break;
  //    case 24 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_24); break;
  //    case 28 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_28); break;
      case 32 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_32); break;
  //    case 36 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_36); break;
  //    case 40 : M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_40); break;
    }
  }else{
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
  }

  return 0;
}

static JSValue esp32_lcd_setFont(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value;
  JS_ToInt32(ctx, &value, argv[0]);
  module_lcd_setFont(value, magic);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_setCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value0;
  int32_t value1;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  if( magic == 1 )
    M5.Displays(g_external_display).setCursor(value0, value1);
  else
    M5.Display.setCursor(value0, value1);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_getCursor(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  JSValue obj = JS_NewObject(ctx);
  if( magic == 1 ){
    JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, M5.Displays(g_external_display).getCursorX()));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, M5.Displays(g_external_display).getCursorY()));
  }else{
    JS_SetPropertyStr(ctx, obj, "x", JS_NewInt32(ctx, M5.Display.getCursorX()));
    JS_SetPropertyStr(ctx, obj, "y", JS_NewInt32(ctx, M5.Display.getCursorY()));
  }
  return obj;
}

static JSValue esp32_lcd_textWidth(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  const char *text = JS_ToCString(ctx, argv[0]);
  int32_t width;
  if( magic == 1 )
    width = M5.Displays(g_external_display).textWidth(text);
  else
    width = M5.Display.textWidth(text);
  JS_FreeCString(ctx, text);
  return JS_NewInt32(ctx, width);
}

static JSValue esp32_lcd_drawPixel(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value0;
  int32_t value1;
  uint32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToUint32(ctx, &value2, argv[2]);
  if( magic == 1 )
    M5.Displays(g_external_display).drawPixel(value0, value1, value2);
  else
    M5.Display.drawPixel(value0, value1, value2);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawLine(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      M5.Displays(g_external_display).drawLine(value0, value1, value2, value3, value4);
    else
      M5.Display.drawLine(value0, value1, value2, value3, value4);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).drawLine(value0, value1, value2, value3);
    else
      M5.Display.drawLine(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawCircle(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value0;
  int32_t value1;
  int32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  if(argc >= 4){
    uint32_t value3;
    JS_ToUint32(ctx, &value3, argv[3]);
    if( magic == 1 )
      M5.Displays(g_external_display).drawCircle(value0, value1, value2, value3);
    else
      M5.Display.drawCircle(value0, value1, value2, value3);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).drawCircle(value0, value1, value2);
    else
      M5.Display.drawCircle(value0, value1, value2);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillCircle(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value0;
  int32_t value1;
  int32_t value2;
  JS_ToInt32(ctx, &value0, argv[0]);
  JS_ToInt32(ctx, &value1, argv[1]);
  JS_ToInt32(ctx, &value2, argv[2]);
  if(argc >= 4){
    uint32_t value3;
    JS_ToUint32(ctx, &value3, argv[3]);
    if( magic == 1 )
      M5.Displays(g_external_display).fillCircle(value0, value1, value2, value3);
    else
      M5.Display.fillCircle(value0, value1, value2, value3);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).fillCircle(value0, value1, value2);
    else
      M5.Display.fillCircle(value0, value1, value2);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      M5.Displays(g_external_display).drawRect(value0, value1, value2, value3, value4);
    else
      M5.Display.drawRect(value0, value1, value2, value3, value4);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).drawRect(value0, value1, value2, value3);
    else
      M5.Display.drawRect(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      M5.Displays(g_external_display).fillRect(value0, value1, value2, value3, value4);
    else
      M5.Display.fillRect(value0, value1, value2, value3, value4);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).fillRect(value0, value1, value2, value3);
    else
      M5.Display.fillRect(value0, value1, value2, value3);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_drawRoundRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      M5.Displays(g_external_display).drawRoundRect(value0, value1, value2, value3, value4, value5);
    else
      M5.Display.drawRoundRect(value0, value1, value2, value3, value4, value5);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).drawRoundRect(value0, value1, value2, value3, value4);
    else
      M5.Display.drawRoundRect(value0, value1, value2, value3, value4);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_fillRoundRect(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

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
    if( magic == 1 )
      M5.Displays(g_external_display).fillRoundRect(value0, value1, value2, value3, value4, value5);
    else
      M5.Display.fillRoundRect(value0, value1, value2, value3, value4, value5);
  }else{
    if( magic == 1 )
      M5.Displays(g_external_display).fillRoundRect(value0, value1, value2, value3, value4);
    else
      M5.Display.fillRoundRect(value0, value1, value2, value3, value4);
  }
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_width(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;
    
  int32_t value;
  if( magic == 1 )
    value = M5.Displays(g_external_display).width();
  else
    value = M5.Display.width();
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_height(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value;
  if( magic == 1 )
    value = M5.Displays(g_external_display).height();
  else
    value = M5.Display.height();
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_fontHeight(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value;
  if( magic == 1 )
    value = M5.Displays(g_external_display).fontHeight();
  else
    value = M5.Display.fontHeight();
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_getColorDepth(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  int32_t value;
  if( magic == 1 )
    value = M5.Displays(g_external_display).getColorDepth();
  else
    value = M5.Display.getColorDepth();
  return JS_NewInt32(ctx, value);
}

static JSValue esp32_lcd_fillScreen(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv, int magic)
{
  if( magic == 1 && g_external_display == -1 )
    return JS_EXCEPTION;

  uint32_t value;
  JS_ToUint32(ctx, &value, argv[0]);
  if( magic == 1 )
    M5.Displays(g_external_display).fillScreen(value);
  else
    M5.Display.fillScreen(value);
  return JS_UNDEFINED;
}

static JSValue esp32_lcd_displayType(JSContext *ctx, JSValueConst jsThis, int argc, JSValueConst *argv)
{
  return JS_NewInt32(ctx, g_external_display_type);
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
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setRotation }}
                         }},
    JSCFunctionListEntry{"setBrightness", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setBrigthness}}
                         }},
    JSCFunctionListEntry{"setFont", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setFont}}
                         }},
    JSCFunctionListEntry{"setTextColor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextColor}}
                         }},
    JSCFunctionListEntry{"setTextSize", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextSize}}
                         }},
    JSCFunctionListEntry{"setTextDatum", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextDatum}}
                         }},
    JSCFunctionListEntry{"drawPixel", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawPixel}}
                         }},
    JSCFunctionListEntry{"drawLine", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawLine}}
                         }},
    JSCFunctionListEntry{"drawCircle", 0, JS_DEF_CFUNC, 0, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawCircle}}
                         }},
    JSCFunctionListEntry{"fillCircle", 0, JS_DEF_CFUNC, 0, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillCircle}}
                         }},
    JSCFunctionListEntry{"drawRect", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawRect}}
                         }},
    JSCFunctionListEntry{"fillRect", 0, JS_DEF_CFUNC, 0, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRect}}
                         }},
    JSCFunctionListEntry{"drawRoundRect", 0, JS_DEF_CFUNC, 0, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawRoundRect}}
                         }},
    JSCFunctionListEntry{"fillRoundRect", 0, JS_DEF_CFUNC, 0, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRoundRect}}
                         }},
    JSCFunctionListEntry{"setCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setCursor}}
                         }},
    JSCFunctionListEntry{"getCursor", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getCursor}}
                         }},
    JSCFunctionListEntry{"textWidth", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_textWidth}}
                         }},
    JSCFunctionListEntry{"print", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"println", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_println}}
                         }},
    JSCFunctionListEntry{"fillScreen", 0, JS_DEF_CFUNC, 0, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillScreen}}
                         }},
#ifdef _SD_ENABLE_
    JSCFunctionListEntry{"drawImageFile", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image_file}}
                         }},
#endif
#ifndef _WIFI_DISABLE_
    JSCFunctionListEntry{"drawImageUrl", 0, JS_DEF_CFUNC, 0, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image_url}}
                         }},
#endif
    JSCFunctionListEntry{"drawImage", 0, JS_DEF_CFUNC, 0, {
                           func : {10, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image}}
                         }},
    JSCFunctionListEntry{"width", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_width}}
                         }},
    JSCFunctionListEntry{"height", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_height}}
                         }},
    JSCFunctionListEntry{"getColorDepth", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getColorDepth}}
                         }},
    JSCFunctionListEntry{"fontHeight", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fontHeight}}
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

static const JSCFunctionListEntry lcd_funcs2[] = {
    JSCFunctionListEntry{"setRotation", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setRotation }}
                         }},
    JSCFunctionListEntry{"setBrightness", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setBrigthness}}
                         }},
    JSCFunctionListEntry{"setFont", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setFont}}
                         }},
    JSCFunctionListEntry{"setTextColor", 0, JS_DEF_CFUNC, 1, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextColor}}
                         }},
    JSCFunctionListEntry{"setTextSize", 0, JS_DEF_CFUNC, 1, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextSize}}
                         }},
    JSCFunctionListEntry{"setTextDatum", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setTextDatum}}
                         }},
    JSCFunctionListEntry{"drawPixel", 0, JS_DEF_CFUNC, 1, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawPixel}}
                         }},
    JSCFunctionListEntry{"drawLine", 0, JS_DEF_CFUNC, 1, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawLine}}
                         }},
    JSCFunctionListEntry{"drawCircle", 0, JS_DEF_CFUNC, 1, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawCircle}}
                         }},
    JSCFunctionListEntry{"fillCircle", 0, JS_DEF_CFUNC, 1, {
                           func : {4, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillCircle}}
                         }},
    JSCFunctionListEntry{"drawRect", 0, JS_DEF_CFUNC, 1, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawRect}}
                         }},
    JSCFunctionListEntry{"fillRect", 0, JS_DEF_CFUNC, 1, {
                           func : {5, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRect}}
                         }},
    JSCFunctionListEntry{"drawRoundRect", 0, JS_DEF_CFUNC, 1, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_drawRoundRect}}
                         }},
    JSCFunctionListEntry{"fillRoundRect", 0, JS_DEF_CFUNC, 1, {
                           func : {6, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillRoundRect}}
                         }},
    JSCFunctionListEntry{"setCursor", 0, JS_DEF_CFUNC, 1, {
                           func : {2, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_setCursor}}
                         }},
    JSCFunctionListEntry{"getCursor", 0, JS_DEF_CFUNC, 1, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getCursor}}
                         }},
    JSCFunctionListEntry{"textWidth", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_textWidth}}
                         }},
    JSCFunctionListEntry{"print", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_print}}
                         }},
    JSCFunctionListEntry{"println", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_println}}
                         }},
    JSCFunctionListEntry{"fillScreen", 0, JS_DEF_CFUNC, 1, {
                           func : {1, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fillScreen}}
                         }},
#ifdef _SD_ENABLE_
    JSCFunctionListEntry{"drawImageFile", 0, JS_DEF_CFUNC, 1, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image_file}}
                         }},
#endif
#ifndef _WIFI_DISABLE_
    JSCFunctionListEntry{"drawImageUrl", 0, JS_DEF_CFUNC, 1, {
                           func : {3, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image_url}}
                         }},
#endif
    JSCFunctionListEntry{"drawImage", 0, JS_DEF_CFUNC, 1, {
                           func : {10, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_draw_image}}
                         }},
    JSCFunctionListEntry{"width", 0, JS_DEF_CFUNC, 1, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_width}}
                         }},
    JSCFunctionListEntry{"height", 0, JS_DEF_CFUNC, 1, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_height}}
                         }},
    JSCFunctionListEntry{"getColorDepth", 0, JS_DEF_CFUNC, 1, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_getColorDepth}}
                         }},
    JSCFunctionListEntry{"fontHeight", 0, JS_DEF_CFUNC, 1, {
                           func : {0, JS_CFUNC_generic_magic, {generic_magic : esp32_lcd_fontHeight}}
                         }},
    JSCFunctionListEntry{"displayType", 0, JS_DEF_CFUNC, 0, {
                           func : {0, JS_CFUNC_generic, esp32_lcd_displayType}
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
    JSCFunctionListEntry{
        "type_m5module_display", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5ModuleDisplay
        }},
    JSCFunctionListEntry{
        "type_m5module_rca", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5ModuleRCA
        }},
    JSCFunctionListEntry{
        "type_m5unit_glass", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitGLASS
        }},
    JSCFunctionListEntry{
        "type_m5unit_glass2", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitGLASS2
        }},
    JSCFunctionListEntry{
        "type_m5unit_oled", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitOLED
        }},
    JSCFunctionListEntry{
        "type_m5unit_mini_oled", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitMiniOLED
        }},
    JSCFunctionListEntry{
        "type_m5unit_lcd", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitLCD
        }},
    JSCFunctionListEntry{
        "type_m5unit_rca", 0, JS_DEF_PROP_INT32, 0, {
          i32 : m5::board_t::board_M5UnitRCA
        }},
    JSCFunctionListEntry{
        "type_not_found", 0, JS_DEF_PROP_INT32, 0, {
          i32 : -1
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

  mod = JS_NewCModule(ctx, "Lcd2", [](JSContext *ctx, JSModuleDef *m){
        return JS_SetModuleExportList(
                            ctx, m, lcd_funcs2,
                            sizeof(lcd_funcs2) / sizeof(JSCFunctionListEntry));
      });
  if (mod){
    JS_AddModuleExportList(
        ctx, mod, lcd_funcs2,
        sizeof(lcd_funcs2) / sizeof(JSCFunctionListEntry));
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

  if( g_external_display != -1 ){
    M5.Displays(g_external_display).setFont(&fonts::lgfxJapanGothic_16);
    M5.Displays(g_external_display).setColorDepth(16);
    M5.Displays(g_external_display).setBrightness(128);
    M5.Displays(g_external_display).setRotation(1);
    M5.Displays(g_external_display).setCursor(0, 0);
    M5.Displays(g_external_display).setTextColor(FONT_COLOR);
  }

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
