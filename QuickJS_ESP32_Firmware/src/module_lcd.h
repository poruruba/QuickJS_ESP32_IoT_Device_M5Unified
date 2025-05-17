#ifndef _MODULE_LCD_H_
#define _MODULE_LCD_H_

#include "module_type.h"

#ifdef _LCD_ENABLE_

extern JsModuleEntry lcd_module;

long module_lcd_setFont(uint16_t size, int magic);

#endif

#endif
