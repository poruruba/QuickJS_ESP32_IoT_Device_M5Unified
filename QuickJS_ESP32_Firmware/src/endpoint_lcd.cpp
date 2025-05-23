#include <Arduino.h>
#include "main_config.h"

#ifdef _LCD_ENABLE_

#ifdef _SD_ENABLE_
#include <SD.h>
#endif

#include "endpoint_types.h"
#include "endpoint_lcd.h"
#include "module_lcd.h"

long endp_lcd_setRotation(JsonObject& request, JsonObject& response, int magic)
{
  int32_t rot = request["rot"];

  M5.Display.setRotation(rot);

  return 0;
}

long endp_lcd_setBrightness(JsonObject& request, JsonObject& response, int magic)
{
  int32_t brt = request["brt"];

  M5.Display.setBrightness(brt);

  return 0;
}

long endp_lcd_setFont(JsonObject& request, JsonObject& response, int magic)
{
  int32_t size = request["size"];

  module_lcd_setFont(size, 0);

  return 0;
}

long endp_lcd_setTextColor(JsonObject& request, JsonObject& response, int magic)
{
  int32_t fore = request["fore"];
  int32_t back = request["back"] || -1;
  if( back >= 0 )
    M5.Display.setTextColor((uint32_t)fore, (uint32_t)back);
  else
    M5.Display.setTextColor((uint32_t)fore);

  return 0;
}

long endp_lcd_setTextSize(JsonObject& request, JsonObject& response, int magic)
{
  float scale = request["scale"];
  float yscale = request["yscale"] || -1.0;
  if( yscale >= 0 )
    M5.Display.setTextSize(scale, yscale);
  else
    M5.Display.setTextSize(scale);

  return 0;
}

long endp_lcd_setTextDatum(JsonObject& request, JsonObject& response, int magic)
{
  int32_t datum = request["datum"];
  M5.Display.setTextDatum(datum);

  return 0;
}

long endp_lcd_drawPixel(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x = request["x"];
  int32_t y = request["y"];
  uint32_t color = request["color"];
  M5.Display.drawPixel(x, y, color);

  return 0;
}

long endp_lcd_drawLine(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x0 = request["x0"];
  int32_t y0 = request["y0"];
  int32_t x1 = request["x1"];
  int32_t y1 = request["y1"];
  uint32_t color = request["color"];
  M5.Display.drawLine(x0, y0, x1, y1, color);

  return 0;
}

long endp_lcd_drawRect(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x = request["x"];
  int32_t y = request["y"];
  int32_t w = request["w"];
  int32_t h = request["h"];
  int32_t color = request["color"] || -1;
  if( color >= 0 ){
    if( magic == ENDPOINT_TYPE_DRAWRECT)
      M5.Display.drawRect(x, y, w, h, (uint32_t)color);
    else if( magic == ENDPOINT_TYPE_FILLRECT)
      M5.Display.fillRect(x, y, w, h, (uint32_t)color);
  }else{
    if( magic == ENDPOINT_TYPE_DRAWRECT)
      M5.Display.drawRect(x, y, w, h);
    else if( magic == ENDPOINT_TYPE_FILLRECT)
      M5.Display.fillRect(x, y, w, h);
  }

  return 0;
}

long endp_lcd_drawRoundRect(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x = request["x"];
  int32_t y = request["y"];
  int32_t w = request["w"];
  int32_t h = request["h"];
  int32_t r = request["r"];
  int32_t color = request["color"] || -1;
  if( color >= 0 ){
    if( magic == ENDPOINT_TYPE_DRAWROUNDRECT)
      M5.Display.drawRoundRect(x, y, w, h, r, (uint32_t)color);
    else if( magic == ENDPOINT_TYPE_FILLROUNDRECT)
      M5.Display.fillRoundRect(x, y, w, h, r, (uint32_t)color);
  }else{
    if( magic == ENDPOINT_TYPE_DRAWROUNDRECT)
      M5.Display.drawRoundRect(x, y, w, h, r);
    else if( magic == ENDPOINT_TYPE_FILLROUNDRECT)
      M5.Display.fillRoundRect(x, y, w, h, r);
  }

  return 0;
}

long endp_lcd_drawCircle(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x = request["x"];
  int32_t y = request["y"];
  int32_t r = request["r"];
  int32_t color = request["color"] || -1;
  if( color >= 0 ){
    if( magic == ENDPOINT_TYPE_DRAWCIRCLE)
      M5.Display.drawCircle(x, y, r, (uint32_t)color);
    else if( magic == ENDPOINT_TYPE_FILLCIRCLE)
      M5.Display.fillCircle(x, y, r, (uint32_t)color);
  }else{
    if( magic == ENDPOINT_TYPE_DRAWCIRCLE)
      M5.Display.drawCircle(x, y, r);
    else if( magic == ENDPOINT_TYPE_FILLCIRCLE)
      M5.Display.fillCircle(x, y, r);
  }

  return 0;
}

long endp_lcd_setCursor(JsonObject& request, JsonObject& response, int magic)
{
  int32_t x = request["x"];
  int32_t y = request["y"];
  M5.Display.setCursor(x, y);

  return 0;
}

long endp_lcd_getCursor(JsonObject& request, JsonObject& response, int magic)
{
  response["result"]["x"] = M5.Display.getCursorX();
  response["result"]["y"] = M5.Display.getCursorY();

  return 0;
}

long endp_lcd_textWidth(JsonObject& request, JsonObject& response, int magic)
{
  const char *text = request["text"];
  if( text == NULL )
    return -1;
    
  int32_t ret = M5.Display.textWidth(text);
  response["result"] = ret;

  return 0;
}

long endp_lcd_print(JsonObject& request, JsonObject& response, int magic)
{
  const char *message = request["message"];
  if( message == NULL )
    return -1;

  if( magic == 0 )
    M5.Display.print(message);
  else
    M5.Display.println(message);

  return 0;
}

long endp_lcd_fillScreen(JsonObject& request, JsonObject& response, int magic)
{
  uint32_t color = request["color"];
  M5.Display.fillScreen(color);

  return 0;
}

#ifdef _SD_ENABLE_
long endp_lcd_drawImageFile(JsonObject& request, JsonObject& response, int magic)
{
  const char* filename = request["filename"];
  if( filename == NULL )
    return -1;
  
  int32_t x = request["x"] || 0;
  int32_t y = request["y"] || 0;

  File file = SD.open(filename, FILE_READ);
  if( !file )
    return -1;

  uint8_t image_buffer[4];
  int size = file.read(image_buffer, sizeof(image_buffer));
  file.close();
  if( size < sizeof(image_buffer) )
    return -1;

  bool ret = false;
  if( image_buffer[0] == 0xff && image_buffer[1] == 0xd8 ){
    ret = M5.Display.drawJpgFile(SD, filename, x, y);
  }else if (image_buffer[0] == 0x89 && image_buffer[1] == 0x50 && image_buffer[2] == 0x4e && image_buffer[3] == 0x47 ){
    ret = M5.Display.drawPngFile(SD, filename, x, y);
  }

  if( !ret )
    return -1;

  return 0;
}
#endif

long endp_lcd_getMetric(JsonObject& request, JsonObject& response, int magic)
{
  int32_t value = 0;

  switch(magic){
    case ENDPOINT_TYPE_WIDTH: value = M5.Display.width(); break;
    case ENDPOINT_TYPE_HEIGHT: value = M5.Display.height(); break;
    case ENDPOINT_TYPE_DEPTH: value = M5.Display.getColorDepth(); break;
    case ENDPOINT_TYPE_FONTHEIGHT: value = M5.Display.fontHeight(); break;
  }

  response["result"] = value;

  return 0;
}

EndpointEntry lcd_table[] = {
  EndpointEntry{ endp_lcd_setRotation, "/lcd-setRotation", -1 },
  EndpointEntry{ endp_lcd_setBrightness, "/lcd-setBrightness", -1 },
  EndpointEntry{ endp_lcd_setFont, "/lcd-setFont", -1 },
  EndpointEntry{ endp_lcd_setTextColor, "/lcd-setTextColor", -1 },
  EndpointEntry{ endp_lcd_setTextSize, "/lcd-setTextSize", -1 },
  EndpointEntry{ endp_lcd_setTextDatum, "/lcd-setTextDatum", -1 },
  EndpointEntry{ endp_lcd_drawPixel, "/lcd-drawPixel", -1 },
  EndpointEntry{ endp_lcd_drawLine, "/lcd-drawLine", -1 },
  EndpointEntry{ endp_lcd_drawRect, "/lcd-drawRect", ENDPOINT_TYPE_DRAWRECT },
  EndpointEntry{ endp_lcd_drawRect, "/lcd-fillRect", ENDPOINT_TYPE_FILLRECT },
  EndpointEntry{ endp_lcd_drawRoundRect, "/lcd-drawRoundRect", ENDPOINT_TYPE_DRAWROUNDRECT },
  EndpointEntry{ endp_lcd_drawRoundRect, "/lcd-fillRoundRect", ENDPOINT_TYPE_FILLROUNDRECT },
  EndpointEntry{ endp_lcd_drawCircle, "/lcd-drawCircle", ENDPOINT_TYPE_DRAWCIRCLE },
  EndpointEntry{ endp_lcd_drawCircle, "/lcd-fillCircle", ENDPOINT_TYPE_FILLCIRCLE },
  EndpointEntry{ endp_lcd_setCursor, "/lcd-setCursor", -1 },
  EndpointEntry{ endp_lcd_getCursor, "/lcd-getCursor", -1 },
  EndpointEntry{ endp_lcd_textWidth, "/lcd-textWidth", -1 },
  EndpointEntry{ endp_lcd_print, "/lcd-print", 0 },
  EndpointEntry{ endp_lcd_print, "/lcd-println", 1 },
  EndpointEntry{ endp_lcd_fillScreen, "/lcd-fillScreen", -1 },
#ifdef _SD_ENABLE_
  EndpointEntry{ endp_lcd_drawImageFile, "/lcd-drawImageFile", -1 },
#endif
  EndpointEntry{ endp_lcd_getMetric, "/lcd-width", ENDPOINT_TYPE_WIDTH },
  EndpointEntry{ endp_lcd_getMetric, "/lcd-height", ENDPOINT_TYPE_HEIGHT },
  EndpointEntry{ endp_lcd_getMetric, "/lcd-getColorDepth", ENDPOINT_TYPE_DEPTH },
  EndpointEntry{ endp_lcd_getMetric, "/lcd-fontHeight", ENDPOINT_TYPE_FONTHEIGHT },
};

const int num_of_lcd_entry = sizeof(lcd_table) / sizeof(EndpointEntry);

#endif
