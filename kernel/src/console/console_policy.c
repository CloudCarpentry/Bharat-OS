#include "console/console_policy.h"
#include <stddef.h>

void console_choose_policy(const console_device_desc_t *devs,
                           size_t count,
                           console_policy_decision_t *decision) {
    if (!decision) return;

    decision->early_primary_index = -1;
    decision->runtime_primary_index = -1;
    decision->runtime_secondary_index = -1;
    decision->panic_primary_index = -1;
    decision->panic_secondary_index = -1;

    if (!devs || count == 0) return;

    for (size_t i = 0; i < count; i++) {
        const console_device_desc_t *dev = &devs[i];

        // Memlog is always an active choice behind the scenes, handled by core init

        // Early Primary: Serial/Debug/SBI preferred
        if (dev->early_ok && (dev->type == CONSOLE_BACKEND_SERIAL || dev->type == CONSOLE_BACKEND_DEBUGPORT || dev->type == CONSOLE_BACKEND_SBI)) {
            if (decision->early_primary_index == -1 || devs[decision->early_primary_index].priority < dev->priority) {
                decision->early_primary_index = i;
            }
        }

        // Runtime Primary: Framebuffer visible sink preferred
        if (dev->caps & CON_CAP_VISIBLE_SINK && dev->type == CONSOLE_BACKEND_FRAMEBUFFER) {
             if (decision->runtime_primary_index == -1 || devs[decision->runtime_primary_index].priority < dev->priority) {
                 decision->runtime_primary_index = i;
             }
        }

        // Runtime Secondary: Serial/SBI
        if (dev->type == CONSOLE_BACKEND_SERIAL || dev->type == CONSOLE_BACKEND_SBI) {
            if (decision->runtime_secondary_index == -1 || devs[decision->runtime_secondary_index].priority < dev->priority) {
                decision->runtime_secondary_index = i;
            }
        }

        // Panic Primary: Serial polling preferred
        if (dev->panic_ok && (dev->caps & CON_CAP_WRITE_POLL)) {
            if (decision->panic_primary_index == -1 || devs[decision->panic_primary_index].priority < dev->priority) {
                 decision->panic_primary_index = i;
            }
        }
    }
}
