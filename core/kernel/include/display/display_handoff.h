#ifndef BHARAT_DISPLAY_HANDOFF_H
#define BHARAT_DISPLAY_HANDOFF_H

#include "bharat/display/boot_video.h"
#include "capability.h"
#include <stdint.h>

typedef enum {
    BH_DISPLAY_HANDOFF_HEADLESS = 0,
    BH_DISPLAY_HANDOFF_KERNEL_OWNED,
    BH_DISPLAY_HANDOFF_PENDING,
    BH_DISPLAY_HANDOFF_QUIESCED,
} bh_display_handoff_state_t;

/* The recipient cspace owns the returned capability for its full lifetime. */
int bh_display_publish_boot_framebuffer(const boot_video_handoff_t *framebuffer,
                                        capability_table_t *recipient,
                                        uint32_t *out_cap);
int bh_display_handoff_begin(const capability_table_t *caller, uint32_t cap);
int bh_display_handoff_commit(const capability_table_t *caller, uint32_t cap);
int bh_display_handoff_abort(const capability_table_t *caller, uint32_t cap);
bh_display_handoff_state_t bh_display_handoff_state(void);

#endif
