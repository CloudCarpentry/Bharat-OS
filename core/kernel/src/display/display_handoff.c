#include "display/display_handoff.h"
#include "console/console_core.h"
#include "bharat/display/display_caps.h"
#include "kernel.h"

/*
 * Boot-core-owned state. Calls occur before SMP lifecycle delegation; the
 * console lock serializes sink mutation at commit. A later cross-core display
 * protocol must replace this ownership rather than remotely mutating it.
 */
static bh_display_handoff_state_t g_handoff_state = BH_DISPLAY_HANDOFF_HEADLESS;
static uint64_t g_framebuffer_phys;

static int validate_authority(const capability_table_t *caller, uint32_t cap) {
    bh_memory_object_t memory = {0};
    kstatus_t status = cap_lookup_memory(caller, cap, CAP_RIGHT_MEMORY_MAP, &memory);
    if (status != K_OK || (uint64_t)memory.base != g_framebuffer_phys) {
        return -1;
    }
    return 0;
}

int bh_display_publish_boot_framebuffer(const boot_video_handoff_t *framebuffer,
                                        capability_table_t *recipient,
                                        uint32_t *out_cap) {
    if (!framebuffer || !framebuffer->valid || !recipient || !out_cap) return -1;
    if (boot_video_validate(framebuffer) != 0) return -2;

    int rc = cap_table_grant(recipient, CAP_TYPE_MEMORY, framebuffer->phys_addr,
                             CAP_RIGHT_MEMORY_MAP, out_cap);
    if (rc != 0) return -3;

    g_framebuffer_phys = framebuffer->phys_addr;
    g_handoff_state = BH_DISPLAY_HANDOFF_KERNEL_OWNED;
    return 0;
}

int bh_display_handoff_begin(const capability_table_t *caller, uint32_t cap) {
    if (g_handoff_state == BH_DISPLAY_HANDOFF_HEADLESS) return -2;
    if (validate_authority(caller, cap) != 0) return -1;
    if (g_handoff_state == BH_DISPLAY_HANDOFF_PENDING) return 0;
    if (g_handoff_state != BH_DISPLAY_HANDOFF_KERNEL_OWNED) return -3;
    g_handoff_state = BH_DISPLAY_HANDOFF_PENDING;
    return 0;
}

int bh_display_handoff_commit(const capability_table_t *caller, uint32_t cap) {
    if (validate_authority(caller, cap) != 0) return -1;
    if (g_handoff_state == BH_DISPLAY_HANDOFF_QUIESCED) return 0;
    if (g_handoff_state != BH_DISPLAY_HANDOFF_PENDING) return -3;

    console_quiesce_framebuffer_sinks();
    g_handoff_state = BH_DISPLAY_HANDOFF_QUIESCED;
    return 0;
}

int bh_display_handoff_abort(const capability_table_t *caller, uint32_t cap) {
    if (validate_authority(caller, cap) != 0) return -1;
    if (g_handoff_state == BH_DISPLAY_HANDOFF_KERNEL_OWNED) return 0;
    if (g_handoff_state != BH_DISPLAY_HANDOFF_PENDING) return -3;
    g_handoff_state = BH_DISPLAY_HANDOFF_KERNEL_OWNED;
    return 0;
}

bh_display_handoff_state_t bh_display_handoff_state(void) {
    return g_handoff_state;
}
