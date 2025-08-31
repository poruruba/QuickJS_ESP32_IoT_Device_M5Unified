#ifndef _MODULE_WEBSOCKET_H_
#define _MODULE_WEBSOCKET_H_

#include "module_type.h"
#include <ESPAsyncWebServer.h>

extern JsModuleEntry websocket_module;

void onWebsocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);

#endif
