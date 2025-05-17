#ifndef _ENDPOINT_LCD_H_
#define _ENDPOINT_LCD_H_

#include "endpoint_types.h"

#define ENDPOINT_TYPE_WIDTH 1
#define ENDPOINT_TYPE_HEIGHT 2
#define ENDPOINT_TYPE_DEPTH 3
#define ENDPOINT_TYPE_FONTHEIGHT 4

#define ENDPOINT_TYPE_DRAWRECT  0
#define ENDPOINT_TYPE_FILLRECT  1
#define ENDPOINT_TYPE_DRAWROUNDRECT  2
#define ENDPOINT_TYPE_FILLROUNDRECT  3
#define ENDPOINT_TYPE_DRAWCIRCLE  4
#define ENDPOINT_TYPE_FILLCIRCLE  5

extern EndpointEntry lcd_table[];
extern const int num_of_lcd_entry;

#endif