#include "hal/hal_capabilities.h"
#include "hal/hal_irq.h"
#include "hal/hal_ipi.h"
#include "kernel/status.h"

int hal_interrupt_controller_init(void) {
    return 0; // Simple stub
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
}

int hal_irq_enable(uint32_t vector) {
    (void)vector;
    return K_ERR_UNSUPPORTED;
}

int hal_irq_disable(uint32_t vector) {
    (void)vector;
    return K_ERR_UNSUPPORTED;
}

uint32_t hal_irq_claim(void) {
    return 0;
}

void hal_irq_eoi(uint32_t irq) {
    (void)irq;
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    /* This target advertises UP-only operation, so no remote owner exists. */
    (void)mask;
    (void)reason;
}
