#ifndef _MEM_UTILS_H_
#define _MEM_UTILS_H_

#include <Arduino.h>

void* utils_mem_alloc(size_t size);
void* utils_mem_realloc(void* buffer, size_t size);
void utils_mem_free(void* buffer);

#endif