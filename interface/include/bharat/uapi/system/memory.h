#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_MEM_CLASS_DEFAULT = 0,
    BHARAT_MEM_CLASS_DMA = 1,
    BHARAT_MEM_CLASS_PACKET = 2,
    BHARAT_MEM_CLASS_SECURE = 3,
    BHARAT_MEM_CLASS_MODEL = 4,
    BHARAT_MEM_CLASS_SENSOR_STREAM = 5,
    BHARAT_MEM_CLASS_RT = 6,
    BHARAT_MEM_CLASS_LOWPOWER = 7,
    BHARAT_MEM_CLASS_PERSISTENT = 8,
} bharat_mem_class_t;

#ifdef __cplusplus
}
#endif
