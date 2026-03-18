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

#include "bharat/boot_info.h"   /* boot_video_handoff_t */
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
int boot_gui_run(void);

/*
 * boot_gui_is_active() — Query whether the boot GUI is currently active.
 * Safe to call from any context after boot_gui_run().
 */
bool boot_gui_is_active(void);

/*
 * boot_gui_get_handoff() — Retrieve the validated framebuffer descriptor.
 * Returns NULL if boot_gui_run() was not called or no framebuffer is active.
 */
const boot_video_handoff_t *boot_gui_get_handoff(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_BOOT_GUI_INIT_H */
