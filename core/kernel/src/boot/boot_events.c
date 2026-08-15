#include "boot/boot_events.h"
#include "display/boot_gui_init.h"
#include "kernel/primitive.h"

// For now, we just pass this through to the GUI, but we can hook this up to the console/log too
void boot_events_publish(bh_boot_stage_t stage, uint8_t percent, bharat_status_t status, const char *label) {
    (void)stage;
    (void)status;
    boot_gui_update_progress(percent, label);
}
