#include "bharat/boot_info.h"
#include <stdint.h>
#include <stdbool.h>

static bool is_overlap(uint64_t start1, uint64_t end1, uint64_t start2, uint64_t end2) {
    if (end1 <= start2) return false;
    if (end2 <= start1) return false;
    return true; // Overlap detected
}

// Function to validate the normalized boot info contract
bool boot_info_validate(const boot_info_t *boot) {
    if (!boot) return false;

    // 1. Kernel bounds ordering and overflow
    if (boot->kernel_phys_end < boot->kernel_phys_start) {
        return false;
    }

    // 2. Check if the memory map is reasonable
    if (boot->mem_map_count > BHARAT_BOOT_MAX_MEM_REGIONS) return false;

    for (uint32_t i = 0; i < boot->mem_map_count; i++) {
        uint64_t r_start = boot->mem_map[i].phys_start;
        uint64_t r_end = r_start + boot->mem_map[i].size;
        if (r_end < r_start) return false; // Overflow in map region

        // If it's a reserved region, it must NOT overlap the kernel
        if (boot->mem_map[i].type == BOOT_MEM_RESERVED) {
            if (is_overlap(r_start, r_end, boot->kernel_phys_start, boot->kernel_phys_end)) {
                return false;
            }
        }
    }

    // 3. Validate module bounds
    for (uint32_t i = 0; i < boot->module_count; i++) {
        uint64_t start = boot->modules[i].phys_start;
        uint64_t end = start + boot->modules[i].size;

        // Overflow check
        if (end < start) return false;

        // Module must not overlap with kernel
        if (is_overlap(start, end, boot->kernel_phys_start, boot->kernel_phys_end)) {
            return false;
        }
    }

    // 4. Strict validation of the video metadata
    if (boot->video.valid) {
        uint64_t fb_start = boot->video.phys_addr;
        uint64_t fb_size = boot->video.size;

        // Avoid overflow in size calculations
        uint64_t fb_end = fb_start + fb_size;
        if (fb_end < fb_start) {
            return false; // Overflow in size calculation
        }

        // Ensure stride, width, height match sanity bounds and do not multiply to an overflow
        if (boot->video.stride_bytes == 0 || boot->video.width == 0 || boot->video.height == 0) {
            return false;
        }

        // Avoid multiplication overflow (stride * height)
        uint64_t calc_size = (uint64_t)boot->video.stride_bytes * boot->video.height;
        // Check if multiplying stride * height wrapped around (e.g. division check)
        if (calc_size / boot->video.stride_bytes != boot->video.height) {
             return false; // Multiplication overflow
        }
        // Ensure size covers at least one frame
        if (calc_size > fb_size) {
            return false;
        }

        // Framebuffer must not overlap with kernel
        if (is_overlap(fb_start, fb_end, boot->kernel_phys_start, boot->kernel_phys_end)) {
            return false;
        }

        // Framebuffer must not overlap with any modules
        for (uint32_t i = 0; i < boot->module_count; i++) {
            if (is_overlap(fb_start, fb_end, boot->modules[i].phys_start, boot->modules[i].phys_start + boot->modules[i].size)) {
                return false;
            }
        }

        // Framebuffer must not overlap with reserved memory
        for (uint32_t i = 0; i < boot->mem_map_count; i++) {
            if (boot->mem_map[i].type == BOOT_MEM_RESERVED) {
                uint64_t r_start = boot->mem_map[i].phys_start;
                uint64_t r_end = r_start + boot->mem_map[i].size;
                if (is_overlap(fb_start, fb_end, r_start, r_end)) {
                    return false;
                }
            }
        }
    }

    // 5. Validate command line termination (string_length safe)
    bool null_terminated = false;
    for (uint32_t i = 0; i < BHARAT_BOOT_CMDLINE_MAX_LEN; i++) {
        if (boot->cmdline[i] == '\0') {
            null_terminated = true;
            break;
        }
    }
    if (!null_terminated) return false;

    return true;
}
