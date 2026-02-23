#ifndef _ENDPOINT_PACKET_H_
#define _ENDPOINT_PACKET_H_

#include <ArduinoJson.h>
#include "endpoint_types.h"

long packet_initialize(void);
void packet_appendEntry(EndpointEntry *tables, int num_of_entry);
long packet_execute(const char *endpoint, JsonObject& params, JsonObject& responseResult);
long packet_open(void);
long packet_close(void);
long packet_isRunning(void);
long packet_set_content(const uint8_t *p_data, size_t len, const char *content_type);
long packet_clear_content(void);

#endif
