#ifndef BHARAT_ISOLATION_H
#define BHARAT_ISOLATION_H

#include <stdint.h>

/*
 * Bharat-OS Isolation Classes
 * Defines authorization levels for privileged resources like MMIO, IRQs, and DMA.
 */

typedef enum {
    ISOLATION_CLASS_USER = 0,
    ISOLATION_CLASS_DRIVER = 1,
    ISOLATION_CLASS_SYSTEM = 2,
    ISOLATION_CLASS_ROOT = 3
} bharat_isolation_class_t;

typedef struct {
    uint32_t process_id;
    uint32_t address_space_id;
    uint32_t sandbox_flags;
    uint8_t service_isolated;
} bharat_isolation_context_t;

typedef struct {
    uint32_t process_id;
    uint32_t allowed_syscall_mask;
    uint32_t allowed_mmio_regions;
} bharat_sandbox_policy_t;

#define BHARAT_SANDBOX_FLAG_SYSCALL_FILTER     (1U << 0)
#define BHARAT_SANDBOX_FLAG_MMIO_RESTRICTED    (1U << 1)
#define BHARAT_SANDBOX_FLAG_SERVICE_CONTAINED  (1U << 2)

int bharat_isolation_init(void);
int bharat_isolation_bind_process(uint32_t process_id,
                                  uint32_t address_space_id,
                                  bharat_isolation_class_t iso_class,
                                  bharat_isolation_context_t* out_ctx);
int bharat_isolation_apply_sandbox(const bharat_sandbox_policy_t* policy,
                                   bharat_isolation_context_t* ctx);
int bharat_isolation_iommu_attach(uint32_t device_id, uint32_t process_id);

#endif // BHARAT_ISOLATION_H
