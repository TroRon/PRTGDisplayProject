#pragma once
#include "esp_heap_caps.h"
#include "esp_log.h"
inline void logMemory(const char* phase) {
    const uint32_t caps[]={MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT};
    const char* names[]={"INTERNAL","DMA","SPIRAM"};
    for(unsigned i=0;i<3;i++)ESP_LOGI("EAGLENET_MEM","%s %s free=%u largest=%u minimum=%u",phase,names[i],
        (unsigned)heap_caps_get_free_size(caps[i]),(unsigned)heap_caps_get_largest_free_block(caps[i]),(unsigned)heap_caps_get_minimum_free_size(caps[i]));
}
