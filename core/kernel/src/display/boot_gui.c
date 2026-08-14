/*
 * boot_gui.c — Early boot framebuffer handoff & registration mechanism
 *
 * Microkernel Rule (AGENTS.md / ADR-035):
 *   - The kernel validates video metadata, maps MMIO, and prepares the handoff capability.
 *   - In-kernel font tables and software pixel rendering are strictly forbidden.
 *   - Visual rendering (splash, diagnostics, progress, recovery) is owned by
 *     userspace services (core/services/system/boot_displayd and core/stacks/ui/lcd/tiny_ui.c).
 */

#include "display/boot_gui_init.h"
#include "bharat/display/display_caps.h"
#include "bharat/display/boot_ui_types.h"
#include "boot/boot_args.h"

// Forward declaration from boot_ui_select.c
extern boot_ui_mode_t boot_ui_resolve_mode(system_profile_t profile);

// Forward declarations for boot_video validation
extern int boot_video_collect(boot_video_handoff_t *out);
extern int boot_video_validate(const boot_video_handoff_t *in);
extern int boot_display_register_from_handoff(const boot_video_handoff_t *handoff);

#include "hal/hal.h"         /* hal_serial_write */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ─── Internal state ──────────────────────────────────────────────────── */

typedef struct {
    bool            active;
    void           *fb;           /* virtual address */
    uint32_t        width;
    uint32_t        height;
    uint32_t        stride;       /* bytes per line */
    pixel_format_t  format;
    uint8_t         bpp;          /* bytes per pixel */
} boot_gui_state_t;

static boot_gui_state_t g_gui = {0};
static boot_video_handoff_t g_gui_handoff = {0};

/* ─── Public API ──────────────────────────────────────────────────────── */

int boot_gui_run(void) {
#if !BHARAT_BOOT_GUI
    /* Feature-off: entire function compiles to a single ret -1.
     * The linker will dead-strip this translation unit on headless targets. */
    return -1;
#else
    /* Idempotency guard */
    if (g_gui.active) return 0;

    /* 1. Resolve UI mode based on hardware + profile */
    boot_ui_mode_t mode = boot_ui_resolve_mode(PROFILE_DESKTOP);
    if (mode < BOOT_UI_SIMPLE_FB) {
        hal_serial_write("  [GUI] Hardware does not support boot framebuffer.\n");
        return -1;
    }

    /* 2. Probe the boot video handoff from the machine layer */
    if (!g_gui_handoff.valid) {
        if (boot_video_collect(&g_gui_handoff) != 0 || !g_gui_handoff.valid) {
            hal_serial_write("  [GUI] No valid framebuffer handoff from bootloader.\n");
            return -1;
        }
    }

    /* 3. Validate geometry */
    if (boot_video_validate(&g_gui_handoff) != 0) {
        hal_serial_write("  [GUI] Framebuffer geometry is invalid.\n");
        return -1;
    }

    hal_serial_write("  [GUI] Video phys=");
    hal_serial_write_hex(g_gui_handoff.phys_addr);
    hal_serial_write(" virt=");
    hal_serial_write_hex(g_gui_handoff.virt_addr);
    hal_serial_write(" size=");
    hal_serial_write_hex(g_gui_handoff.size);
    hal_serial_write("\n");

    /* 4. Set up state */
    uint8_t bpp = 0;
    if (g_gui_handoff.format == PIXEL_FORMAT_ARGB8888 ||
        g_gui_handoff.format == PIXEL_FORMAT_XRGB8888 ||
        g_gui_handoff.format == PIXEL_FORMAT_BGRX8888) {
        bpp = 4U;
    } else if (g_gui_handoff.format == PIXEL_FORMAT_RGB565) {
        bpp = 2U;
    } else {
        hal_serial_write("  [GUI] Unsupported pixel format — falling back to text.\n");
        return -1;
    }

    g_gui.fb      = (void *)(uintptr_t)g_gui_handoff.virt_addr;
    g_gui.width   = g_gui_handoff.width;
    g_gui.height  = g_gui_handoff.height;
    g_gui.stride  = g_gui_handoff.stride_bytes;
    g_gui.format  = g_gui_handoff.format;
    g_gui.bpp     = bpp;
    g_gui.active  = true;

    /* Register the display device with the generic display subsystem */
    boot_display_register_from_handoff(&g_gui_handoff);
    hal_serial_write("  [GUI] Boot framebuffer handoff initialized for userspace displayd.\n");

    return 0;
#endif /* BHARAT_BOOT_GUI */
}

bool boot_gui_is_active(void) {
    return g_gui.active;
}

const boot_video_handoff_t *boot_gui_get_handoff(void) {
    if (!g_gui.active) return NULL;
    return &g_gui_handoff;
}

void boot_gui_update_progress(uint32_t percent, const char *status_msg) {
    (void)percent;
    (void)status_msg;
}

/*
 * Export a pointer to our internal handoff state so that boot_video_map.c
 * can update the virtual address after the MMU is initialized.
 */
boot_video_handoff_t* boot_video_get_handoff_ptr(void) {
    return &g_gui_handoff;
}
