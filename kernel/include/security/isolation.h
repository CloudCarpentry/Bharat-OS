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
#define BHARAT_SANDBOX_FLAG_SERVICE_CONTAINED  (1U << 1)
#define BHARAT_SANDBOX_FLAG_DMA_RESTRICTED     (1u << 2)
#define BHARAT_SANDBOX_FLAG_MMIO_RESTRICTED    (1u << 3)
#define BHARAT_SANDBOX_FLAG_PIO_RESTRICTED     (1u << 4)
#define BHARAT_SANDBOX_FLAG_IRQ_RESTRICTED     (1u << 5)
#define BHARAT_SANDBOX_FLAG_NO_DIRECT_MAP      (1u << 6)
#define BHARAT_SANDBOX_FLAG_NO_KERNEL_SYMBOLS  (1u << 7)
#define BHARAT_SANDBOX_FLAG_SIGNED_DRIVER_ONLY (1u << 8)
#define BHARAT_SANDBOX_FLAG_USERSPACE_DRIVER   (1u << 9)

typedef enum {
    BHARAT_DRIVER_TRUST_CORE = 0,
    BHARAT_DRIVER_TRUST_SIGNED,
    BHARAT_DRIVER_TRUST_THIRD_PARTY,
    BHARAT_DRIVER_TRUST_UNTRUSTED,
} bharat_driver_trust_level_t;

typedef enum {
    BHARAT_IOMMU_CAP_DMA_REMAP        = 1u << 0,
    BHARAT_IOMMU_CAP_INT_REMAP        = 1u << 1,
    BHARAT_IOMMU_CAP_PASID            = 1u << 2,
    BHARAT_IOMMU_CAP_PRI              = 1u << 3,
    BHARAT_IOMMU_CAP_SVA              = 1u << 4,
} bharat_iommu_cap_flags_t;

typedef struct bharat_iommu_domain bharat_iommu_domain_t;
typedef struct bharat_iommu_group  bharat_iommu_group_t;
typedef struct bharat_device       bharat_device_t;

typedef enum {
    BHARAT_IOMMU_DOMAIN_KERNEL = 0,
    BHARAT_IOMMU_DOMAIN_DRIVER,
    BHARAT_IOMMU_DOMAIN_USER,
    BHARAT_IOMMU_DOMAIN_BLOCKED,
} bharat_iommu_domain_type_t;

typedef struct {
    bharat_iommu_domain_type_t type;
    uint32_t flags;
} bharat_iommu_domain_config_t;

int bharat_isolation_init(void);
int bharat_isolation_bind_process(uint32_t process_id,
                                  uint32_t address_space_id,
                                  bharat_isolation_class_t iso_class,
                                  bharat_isolation_context_t* out_ctx);
int bharat_isolation_apply_sandbox(const bharat_sandbox_policy_t* policy,
                                   bharat_isolation_context_t* ctx);

int bharat_iommu_domain_create(const bharat_iommu_domain_config_t* cfg,
                               bharat_iommu_domain_t** out_domain);
int bharat_iommu_domain_destroy(bharat_iommu_domain_t* domain);

int bharat_iommu_group_attach(bharat_iommu_group_t* group,
                              bharat_iommu_domain_t* domain);
int bharat_iommu_group_detach(bharat_iommu_group_t* group);

#include <stddef.h>
int bharat_iommu_map(bharat_iommu_domain_t* domain,
                     uint64_t iova,
                     uint64_t phys,
                     size_t size,
                     uint64_t prot_flags);
int bharat_iommu_unmap(bharat_iommu_domain_t* domain,
                       uint64_t iova,
                       size_t size);

int bharat_iommu_block_device(bharat_device_t* dev);
int bharat_iommu_get_group(bharat_device_t* dev,
                           bharat_iommu_group_t** out_group);

#endif // BHARAT_ISOLATION_H
