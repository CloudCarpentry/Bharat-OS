#include "mon_vm_state.h"
#include "../../include/hal/hal.h"
#include "../../include/hal/hal_timer.h"
#include "../../include/urpc/urpc_bootstrap.h"

mon_vm_core_state_t g_mon_vm_core_states[MON_VM_MAX_CPUS] = {0};

// Legacy backward-compatibility shim instance for existing tests/references
mon_vm_state_t g_mon_vm_state = {0};

mon_vm_core_state_t* mon_vm_get_local_state(void) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= MON_VM_MAX_CPUS) {
        core_id = 0; // Fallback
    }
    return &g_mon_vm_core_states[core_id];
}

// Convert monotonic ticks to milliseconds
uint64_t mon_vm_ticks_to_ms(uint64_t ticks) {
    extern uint64_t hal_timer_read_freq(void);
    uint64_t freq = hal_timer_read_freq();
    if (freq == 0) return 0;
    return (ticks * 1000) / freq;
}

// Convert milliseconds to monotonic ticks
uint64_t mon_vm_ms_to_ticks(uint64_t ms) {
    extern uint64_t hal_timer_read_freq(void);
    uint64_t freq = hal_timer_read_freq();
    if (freq == 0) return 0;
    return (ms * freq) / 1000;
}
