#ifndef _MODULE_HTTP_H_
#define _MODULE_HTTP_H_

#include "module_type.h"

extern JsModuleEntry http_module;

#ifdef _HTTP_CUSTOMCALL_
#include <ESPAsyncWebServer.h>
long http_delegateRequest(AsyncWebServerRequest *request, const char *message);

bool http_isWaitRequest(void);
#endif

#endif
