#include "hal/hal_hw_caps.h"
#include "hal/hal_internal.h"
#include <string.h>

static hal_hw_caps_t g_internal_hw_caps;
static bool g_caps_initialized = false;

const hal_hw_caps_t *hal_get_internal_hw_caps(void) {
    return &g_internal_hw_caps;
}

// Internal HAL-only API to set the caps during discovery
void hal_set_internal_hw_caps(const hal_hw_caps_t *caps) {
    if (caps) {
        g_internal_hw_caps = *caps;
        g_caps_initialized = true;
    }
}
