#include "bharat/boot_info.h"
#include "bharat/display/display_caps.h"
#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Collect display info by asking the machine-specific module to provide a
// boot_video_handoff_t.
int boot_video_collect(boot_video_handoff_t *out) {
    if (!out) return -1;
    out->valid = false;

    // machine_probe_boot_video is provided by each board/arch implementation.
    display_probe_result_t probe_res = {0};
    int ret = machine_probe_boot_video(&probe_res, out);
    if (ret != 0 || !probe_res.usable) {
        return -1; // Fallback
    }
    return 0;
}

int boot_video_validate(const boot_video_handoff_t *in) {
    if (!in || !in->valid) return -1;

    // Check basic dimensions
    if (in->width == 0 || in->height == 0 || in->stride_bytes == 0) return -2;

    // Check memory
    if (in->phys_addr == 0 || in->size == 0) return -3;

    // Check minimum expected size matching geometry
    uint32_t expected_size = in->height * in->stride_bytes;
    if (in->size < expected_size) return -4;

    return 0; // Valid
}
