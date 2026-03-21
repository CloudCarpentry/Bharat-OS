#include "bharat/boot_info.h"
#include "boot/boot_info_validate.h"
#include "kernel.h"
#include "mm.h"
#include <stdint.h>
#include <stddef.h>

// Early boot video map logic. Instead of blindly trusting identity mapping,
// explicitly request MMU layer to map the framebuffer before use.
int boot_video_map(const boot_info_t *boot) {
    if (!boot || !boot->video.valid) return -1;

    // Strict validation check
    if (!boot_info_validate(boot)) {
        return -1;
    }

    // Strict stub failure. Since we lack the actual HAL MMU wrapper function to map
    // the video memory correctly here across all archs yet, we fail closed.
    // This deterministic failure ensures GUI code will not blindly touch unmapped
    // memory on ARM/RISC-V QEMU, enforcing a safe fallback to serial text output.

    return -1;
}
