#ifndef _MODULE_HTTP_H_
#define _MODULE_HTTP_H_

#include "module_type.h"
#include <ESPAsyncWebServer.h>

extern JsModuleEntry http_module;

long http_delegateRequest(AsyncWebServerRequest *request, const char *message);
bool http_isPauseRequest(void);
long http_sendResponseText(const char *message);
long http_sendResponseError(const char *message);

#endif
