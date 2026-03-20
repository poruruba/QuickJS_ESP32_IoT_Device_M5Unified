#ifndef _MODULE_SD_H_
#define _MODULE_SD_H_

#include "module_type.h"

//#define SD_TYPE_MMC

#ifdef SD_TYPE_MMC
#define sd  SD_MMC
#else
#define sd  SD
#endif

extern JsModuleEntry sd_module;

#endif
