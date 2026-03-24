#include "boot/boot_info.h"
#include "boot/boot_validate.h"
#include "kernel.h"
#include "mm.h"
#include <stdint.h>
#include <stddef.h>

// Early boot video map logic. Instead of blindly trusting identity mapping,
// explicitly request MMU layer to map the framebuffer before use.
int boot_video_map(const boot_info_t *boot) {
    if (!boot) return -1;

    // For now we check if there's any console framebuffer available from the canonical struct
    if (boot->console.type != BOOT_CONSOLE_FRAMEBUFFER) return -1;

    // Strict validation check
    boot_validation_report_t report;
    if (boot_validate_console(boot, &report) != BOOT_OK) {
        return -1;
    }

    // Strict stub failure. Since we lack the actual HAL MMU wrapper function to map
    // the video memory correctly here across all archs yet, we fail closed.
    // This deterministic failure ensures GUI code will not blindly touch unmapped
    // memory on ARM/RISC-V QEMU, enforcing a safe fallback to serial text output.

    return -1;
}
