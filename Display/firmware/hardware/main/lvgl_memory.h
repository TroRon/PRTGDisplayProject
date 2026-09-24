#pragma once
#include "esp_heap_caps.h"
// LVGL objects, labels and temporary render allocations are CPU-only data.
// Never consume the internal/DMA reserve as a silent PSRAM fallback.
static inline void* panel_lvgl_alloc(size_t bytes) {
    return heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
}
static inline void* panel_lvgl_realloc(void* ptr,size_t bytes) {
    return heap_caps_realloc(ptr,bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
}
static inline void panel_lvgl_free(void* ptr) {heap_caps_free(ptr);}
