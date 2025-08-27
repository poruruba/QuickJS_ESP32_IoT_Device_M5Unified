#ifndef _MODULE_ESP32_H_
#define _MODULE_ESP32_H_

#include "module_type.h"

extern JsModuleEntry esp32_module;
extern JsModuleEntry console_module;

extern int g_external_display;
extern int g_external_display_type;

long esp32_initialize(void);
void esp32_update(void);
uint32_t esp32_getDeviceModel(void);

// long syslog_send(uint16_t pri, const char *p_message);
// long syslog_changeServer(const char *host, uint16_t port);

#endif
