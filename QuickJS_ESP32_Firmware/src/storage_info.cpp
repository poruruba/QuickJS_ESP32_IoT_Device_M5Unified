#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"

unsigned long getRamUsed(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

  unsigned long used  = info.total_allocated_bytes;
  return used;
}

unsigned long getRamTotal(void) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);

  unsigned long total = info.total_free_bytes + info.total_allocated_bytes;
  return total;
}

unsigned long getPartitionApplication(void) {
  const esp_partition_t* running = esp_ota_get_running_partition();
  return running->size;
}

unsigned long getFlashSize(void) {
  uint32_t flash_size = 0;
  esp_flash_get_size(NULL, &flash_size);
  return flash_size;
}
