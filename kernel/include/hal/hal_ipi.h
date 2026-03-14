#ifndef BHARAT_HAL_IPI_H
#define BHARAT_HAL_IPI_H

#include <stdint.h>

typedef enum {
    HAL_IPI_NOP = 0,
    HAL_IPI_RESCHEDULE,
    HAL_IPI_TLB_SHOOTDOWN,
    HAL_IPI_URPC_NOTIFY,
    HAL_IPI_STOP,
    HAL_IPI_PANIC_SYNC,
} hal_ipi_reason_t;

void hal_ipi_init_cpu_local(uint32_t cpu_id);
void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason);
void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason);

#endif // BHARAT_HAL_IPI_H
