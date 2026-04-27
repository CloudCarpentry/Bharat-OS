#ifndef BHARAT_UAPI_SYSTEM_FOOTPRINT_H
#define BHARAT_UAPI_SYSTEM_FOOTPRINT_H
#include <stdint.h>
struct bharat_footprint_stats {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t kernel_pages;
    uint64_t user_pages;
};
#endif
