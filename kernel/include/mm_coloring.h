#ifndef BHARAT_MM_COLORING_H
#define BHARAT_MM_COLORING_H

#include <stdint.h>

#ifndef CONFIG_MM_CACHE_COLORS_DEFAULT
#define CONFIG_MM_CACHE_COLORS_DEFAULT 8
#endif

typedef enum {
    MM_COLOR_POLICY_NONE = 0,
    MM_COLOR_POLICY_PREFERRED,
    MM_COLOR_POLICY_STRICT,
    MM_COLOR_POLICY_ISOLATED
} mm_color_policy_t;

typedef enum {
    MM_DOMAIN_DEFAULT = 0,
    MM_DOMAIN_REALTIME,
    MM_DOMAIN_LOW_LATENCY,
    MM_DOMAIN_BEST_EFFORT,
    MM_DOMAIN_DMA_COHERENT
} mm_domain_t;

typedef struct {
    mm_color_policy_t policy;
    mm_domain_t domain;
    uint32_t color_mask;
} mm_color_config_t;

#endif // BHARAT_MM_COLORING_H
