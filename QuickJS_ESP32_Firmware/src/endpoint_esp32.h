#ifndef _ENDPOINT_ESP32_H_
#define _ENDPOINT_ESP32_H_

#include "endpoint_types.h"

long webcall_putText(const JsonObject& request);

extern EndpointEntry esp32_table[];
extern const int num_of_esp32_entry;

#endif