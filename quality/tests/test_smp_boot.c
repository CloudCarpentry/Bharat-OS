#include <stdio.h>
#include <assert.h>
#include <setjmp.h>
#include "hal/hal_boot.h"
#include "urpc/urpc_bootstrap.h"
#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "hal/hal.h"

// Test flags
static int arch_early_called = 0;
static int irq_init_called = 0;
static int timer_init_called = 0;
static int arch_late_called = 0;
static int hal_cpu_halt_called = 0;

static jmp_buf escape_buf;

// Mocks
uint32_t hal_cpu_get_id(void) { return 1; }
void secondary_entry_arch_early(void) { arch_early_called = 1; }
int hal_irq_init_cpu_local(void) { irq_init_called = 1; return 0; }
int hal_timer_init_cpu_local(uint32_t tick_hz) { timer_init_called = 1; return 0; }
void secondary_entry_arch_late(void) { arch_late_called = 1; }
void hal_cpu_halt(void) { hal_cpu_halt_called = 1; longjmp(escape_buf, 1); }

// Mock boot info
static bharat_boot_info_t mock_boot_info = {
    .cpu_count = 2,
    .cpus = { { .cpu_id = 0, .is_bsp = true }, { .cpu_id = 1, .is_bsp = false } }
};

bharat_boot_info_t* hal_boot_get_info(void) {
    return &mock_boot_info;
}

// Function under test
extern void secondary_entry_common(void);

int main(void) {
    printf("[TEST] Running SMP Boot Sequencing Tests...\n");

    // Catch the infinite loop using longjmp
    if (setjmp(escape_buf) == 0) {
        secondary_entry_common();
    }

    assert(arch_early_called == 1);
    assert(irq_init_called == 1);
    assert(timer_init_called == 1);
    assert(arch_late_called == 1);
    assert(urpc_is_ready(1) == 1);
    assert(hal_cpu_halt_called == 1);

    printf("[TEST] SMP Boot Sequencing Passed.\n");
    return 0;
}
