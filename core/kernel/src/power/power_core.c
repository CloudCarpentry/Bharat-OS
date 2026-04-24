#include "power/power.h"
#include <stddef.h>

/* Core initialization for the power framework.
 * For Phase 1, there's not much state to initialize globally,
 * but this serves as the anchor point for the power/thermal subsystem.
 */

void power_core_init(void) {
    /* Initialize subsystem-wide locks, registries, etc. here in the future. */
}
