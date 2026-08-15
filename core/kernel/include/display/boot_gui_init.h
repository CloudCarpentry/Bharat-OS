/*
 * boot_gui_init.h — Boot-time GUI initialisation API
 *
 * THIS HEADER MUST REMAIN KERNEL-CORE-CLEAN:
 *   - Do NOT include kernel.h, mm.h, or any subsystem header here.
 *   - The kernel core (main.c) includes only this file for display init.
 *   - When BHARAT_BOOT_GUI=0, boot_gui_run() is a no-op stub, so the linker
 *     eliminates the entire display library from the final binary.
 */
#ifndef BHARAT_BOOT_GUI_INIT_H
#define BHARAT_BOOT_GUI_INIT_H

#include "bharat/display/boot_video.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * boot_gui_run() — Probe hardware, resolve UI mode, and activate the
 *                  early framebuffer if supported.
 *
 * Returns:
 *   0   — framebuffer GUI active (pixels are being rendered)
 *  -1   — no GUI support on this hardware/profile (text mode only)
 *
 * This is idempotent: calling it more than once is safe (second call is
 * a no-op returning the same result as the first).
 */
#if defined(BHARAT_BOOT_GUI) && BHARAT_BOOT_GUI
int boot_gui_run(void);
bool boot_gui_is_active(void);
const boot_video_handoff_t *boot_gui_get_handoff(void);
void boot_gui_update_progress(uint32_t percent, const char *status_msg);
#else
static inline int boot_gui_run(void) { return -1; }
static inline bool boot_gui_is_active(void) { return false; }
static inline const boot_video_handoff_t *boot_gui_get_handoff(void) { return NULL; }
static inline void boot_gui_update_progress(uint32_t percent, const char *status_msg) {
    (void)percent;
    (void)status_msg;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_BOOT_GUI_INIT_H */
