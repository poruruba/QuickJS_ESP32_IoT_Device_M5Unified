#include <Arduino.h>
#include <esp_heap_caps.h>

void* utils_mem_alloc(size_t size)
{
  if( psramInit() ){
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }else{
    return malloc(size);
  }
}

void* utils_mem_realloc(void* buffer, size_t size)
{
  if( psramInit() ){
    return heap_caps_realloc(buffer, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }else{
    return realloc(buffer, size);
  }
}

void utils_mem_free(void* buffer)
{
  free(buffer);
}
