#include "hal/hal_ipi.h"
#include "hal/hal_boot.h"
#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "hal/hal_topology.h"
#include "hal/hal.h"
#include "urpc/urpc_bootstrap.h"

// Basic FSM states
typedef enum {
    BOOT_EARLY = 0,
    BOOT_POLICY_APPLIED,
    BOOT_IRQ_READY,
    BOOT_TIMER_READY,
    BOOT_URPC_READY,
    BOOT_SCHED_READY,
    BOOT_ONLINE,
    BOOT_FAILED
} boot_state_t;

static boot_state_t g_core_state[BHARAT_MAX_CPUS];

// Externs for things not defined yet
extern void secondary_entry_arch_early(void);
extern void secondary_entry_arch_late(void);

// Common secondary entry point called by architecture specific code
void secondary_entry_common(void) {
    uint32_t core_id = hal_cpu_get_id();

    // Architecture specific early initialization
    secondary_entry_arch_early();
    g_core_state[core_id] = BOOT_EARLY;

    // Local interrupt controller init
    hal_irq_init_cpu_local(core_id);
    hal_ipi_init_cpu_local(core_id);
    g_core_state[core_id] = BOOT_IRQ_READY;

    // Local timer init
    hal_timer_init_cpu_local(core_id);
    g_core_state[core_id] = BOOT_TIMER_READY;

    // Bind URPC
    if (urpc_bootstrap_core(core_id) != 0) {
        g_core_state[core_id] = BOOT_FAILED;
        goto halt_loop;
    }
    g_core_state[core_id] = BOOT_URPC_READY;
    urpc_mark_ready(core_id);

    // Mark as online
    g_core_state[core_id] = BOOT_ONLINE;

    // Architecture specific late initialization
    secondary_entry_arch_late();

halt_loop:
    // Loop forever until scheduler takes over or if initialization failed
    while (1) {
        hal_cpu_halt();
    }
}
