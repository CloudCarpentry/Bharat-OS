#include "device.h"
#include "kernel_safety.h"
#include "mm.h"
#include "advanced/algo_matrix.h"

#include <stddef.h>

#define MAX_DEV_DRIVERS 32U
#define MAX_MMIO_WINDOWS 64U

static device_driver_t g_drivers[MAX_DEV_DRIVERS];
static device_mmio_window_t g_windows[MAX_MMIO_WINDOWS];

int device_framework_init(void) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        g_drivers[i].name = NULL;
        g_drivers[i].probe = NULL;
        g_drivers[i].irq_handler = NULL;
        g_drivers[i].ctx = NULL;
        g_drivers[i].device_id = 0U;
        g_drivers[i].class_id = DEVICE_CLASS_UART;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        g_windows[i].in_use = 0U;
    }

    return 0;
}

int device_register_driver(const device_driver_t* driver) {
    if (!driver || !driver->name) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (!g_drivers[i].name) {
            g_drivers[i] = *driver;
            if (g_drivers[i].probe) {
                return g_drivers[i].probe();
            }
            return 0;
        }
    }

    return -2;
}

int device_register_mmio_window(const device_mmio_window_t* window) {
    if (!window || window->size_bytes == 0U) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use == 0U) {
            g_windows[i] = *window;
            g_windows[i].in_use = 1U;

            // Map physical to virtual space using VMM
            // Only map if vmm_map_device_mmio is defined/available. For tests we mock/skip.
            #ifndef TESTING
            uint32_t num_pages = (window->size_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint32_t p = 0; p < num_pages; p++) {
                vmm_map_device_mmio(
                    window->virt_base + (p * PAGE_SIZE),
                    window->phys_base + (p * PAGE_SIZE),
                    NULL, // Passing NULL bypasses capabilities for generic MMIO
                    0 // is_npu flag
                );
            }
            #endif

            return 0;
        }
    }

    return -2;
}

// Level 0: Reference generic O(N) linear search
int device_lookup_mmio_window_l0(uint32_t class_id,
                                 uint32_t device_id,
                                 uint32_t window_id,
                                 void* out_window) {
    device_mmio_window_t* out = (device_mmio_window_t*)out_window;
    if (!out) {
        return -1;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_windows); ++i) {
        if (g_windows[i].in_use != 0U &&
            g_windows[i].class_id == class_id &&
            g_windows[i].device_id == device_id &&
            g_windows[i].window_id == window_id) {
            *out = g_windows[i];
            return 0;
        }
    }

    return -2;
}

// Level 1: Optimized lookup (placeholder for an SMP-aware/hashed lookup logic)
// In a full implementation, this might use per-class lock-free hash tables or an RCU list.
// For PoC, we will implement it as a slightly optimized search (e.g. searching backwards or unrolling).
int device_lookup_mmio_window_l1(uint32_t class_id,
                                 uint32_t device_id,
                                 uint32_t window_id,
                                 void* out_window) {
    device_mmio_window_t* out = (device_mmio_window_t*)out_window;
    if (!out) {
        return -1;
    }

    // Optimization: we could track how many are active and only scan that many,
    // or unroll the loop. Let's do a simple reverse scan for demonstration of L1 logic.
    for (int i = BHARAT_ARRAY_SIZE(g_windows) - 1; i >= 0; --i) {
        if (g_windows[i].in_use != 0U &&
            g_windows[i].class_id == class_id &&
            g_windows[i].device_id == device_id &&
            g_windows[i].window_id == window_id) {
            *out = g_windows[i];
            return 0;
        }
    }

    return -2;
}

int device_lookup_mmio_window(device_class_t class_id,
                              uint32_t device_id,
                              uint32_t window_id,
                              device_mmio_window_t* out_window) {
    if (g_search_ops.device_lookup_mmio_window) {
        return g_search_ops.device_lookup_mmio_window(class_id, device_id, window_id, (void*)out_window);
    }
    // Fallback if matrix not initialized
    return device_lookup_mmio_window_l0(class_id, device_id, window_id, (void*)out_window);
}

int device_dispatch_irq(uint32_t irq) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (g_drivers[i].name && g_drivers[i].irq_handler) {
            g_drivers[i].irq_handler(irq, g_drivers[i].ctx);
        }
    }

    return 0;
}

int device_driver_registered(device_class_t class_id, uint32_t device_id) {
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_drivers); ++i) {
        if (g_drivers[i].name != NULL &&
            g_drivers[i].class_id == class_id &&
            g_drivers[i].device_id == device_id) {
            return 1;
        }
    }
    return 0;
}
