#ifndef BHARAT_DISPLAY_CAPS_H
#define BHARAT_DISPLAY_CAPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bharat/display/boot_video.h"

typedef enum {
    CAP_DISPLAY_FB = 1,
    CAP_DISPLAY_MODESET,
    CAP_DISPLAY_INPUT,
    CAP_DISPLAY_SURFACE,
    CAP_DISPLAY_DMA_BUFFER
} display_cap_type_t;

typedef struct machine_display_caps {
    bool display_present;
    bool boot_gui_allowed;
    bool firmware_fb_possible;
    bool early_simplefb_possible;
    bool early_panel_init_possible;
    bool needs_gpu_firmware;
    bool needs_complex_modeset;
    bool input_present;

    uintptr_t mmio_base;
    size_t mmio_size;
    int irq;

    uint32_t max_width;
    uint32_t max_height;
} machine_display_caps_t;

typedef struct display_probe_result {
    bool usable;
    boot_video_path_t path;
    int quality_score;   // higher is better
    bool early_usable;
    bool interactive;
    bool requires_takeover;
} display_probe_result_t;

// Implemented by the board/machine layer
int machine_get_display_caps(machine_display_caps_t *out);
int machine_probe_boot_video(display_probe_result_t *out, boot_video_handoff_t *video);

#endif // BHARAT_DISPLAY_CAPS_H
