#pragma once
#include <stdint.h>
#include <stdio.h>
#include <initializer_list>
#include "esp_timer.h"
inline uint32_t millis(){return (uint32_t)(esp_timer_get_time()/1000);}
