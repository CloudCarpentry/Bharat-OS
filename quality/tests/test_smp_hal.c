#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "hal/hal_irq.h"
#include "hal/hal_timer.h"
#include "hal/hal_ipi.h"

//
// Mocks tracking state for the HAL contracts
//

#define MAX_MOCK_CORES 4

static bool g_irq_boot_inited = false;
static bool g_timer_boot_inited = false;

static bool g_irq_cpu_inited[MAX_MOCK_CORES] = {false};
static bool g_timer_cpu_inited[MAX_MOCK_CORES] = {false};
static bool g_ipi_cpu_inited[MAX_MOCK_CORES] = {false};

static uint64_t g_last_ipi_mask = 0;
static hal_ipi_reason_t g_last_ipi_reason = HAL_IPI_NOP;

// Mock implementations
void hal_irq_init_boot(void) {
    g_irq_boot_inited = true;
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id >= MAX_MOCK_CORES) return;
    g_irq_cpu_inited[cpu_id] = true;
}

void hal_timer_init(void) {
    g_timer_boot_inited = true;
}

void hal_timer_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id >= MAX_MOCK_CORES) return;
    g_timer_cpu_inited[cpu_id] = true;
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    if (cpu_id >= MAX_MOCK_CORES) return;
    g_ipi_cpu_inited[cpu_id] = true;
}

// These mocks should catch uninitialized usage
void hal_timer_program_periodic(uint64_t ns) {
    (void)ns;
    // For test simplicity, assume calling from CPU 0
    assert(g_timer_cpu_inited[0] == true);
}

void hal_timer_program_oneshot(uint64_t ns) {
    (void)ns;
    assert(g_timer_cpu_inited[0] == true);
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    // Cannot send if sender not inited (assume CPU 0 is sending)
    assert(g_ipi_cpu_inited[0] == true);

    // Target must be valid and inited
    assert(target_cpu < MAX_MOCK_CORES);
    assert(g_ipi_cpu_inited[target_cpu] == true);

    g_last_ipi_mask = (1ULL << target_cpu);
    g_last_ipi_reason = reason;
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    assert(g_ipi_cpu_inited[0] == true);

    // Verify all targets in mask are inited
    for (uint32_t i = 0; i < MAX_MOCK_CORES; i++) {
        if (mask & (1ULL << i)) {
            assert(g_ipi_cpu_inited[i] == true);
        }
    }

    g_last_ipi_mask = mask;
    g_last_ipi_reason = reason;
}

static void reset_mocks(void) {
    g_irq_boot_inited = false;
    g_timer_boot_inited = false;
    for (int i=0; i<MAX_MOCK_CORES; i++) {
        g_irq_cpu_inited[i] = false;
        g_timer_cpu_inited[i] = false;
        g_ipi_cpu_inited[i] = false;
    }
    g_last_ipi_mask = 0;
    g_last_ipi_reason = HAL_IPI_NOP;
}

static void test_init_order(void) {
    reset_mocks();
    hal_irq_init_boot();
    hal_timer_init();

    assert(g_irq_boot_inited);
    assert(g_timer_boot_inited);

    hal_irq_init_cpu_local(0);
    hal_timer_init_cpu_local(0);
    hal_ipi_init_cpu_local(0);

    assert(g_irq_cpu_inited[0]);
    assert(g_timer_cpu_inited[0]);
    assert(g_ipi_cpu_inited[0]);
}

static void test_ipi_targeting(void) {
    reset_mocks();

    hal_ipi_init_cpu_local(0);
    hal_ipi_init_cpu_local(1);

    hal_ipi_send(1, HAL_IPI_RESCHEDULE);
    assert(g_last_ipi_mask == (1ULL << 1));
    assert(g_last_ipi_reason == HAL_IPI_RESCHEDULE);

    hal_ipi_broadcast((1ULL << 0) | (1ULL << 1), HAL_IPI_TLB_SHOOTDOWN);
    assert(g_last_ipi_mask == 3);
    assert(g_last_ipi_reason == HAL_IPI_TLB_SHOOTDOWN);
}

int main(void) {
    printf("[TEST] Running SMP HAL Tests...\n");
    test_init_order();
    test_ipi_targeting();
    printf("[TEST] All SMP HAL Tests Passed.\n");
    return 0;
}
