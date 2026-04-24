#ifndef BHARAT_HAL_IOMMU_H
#define BHARAT_HAL_IOMMU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t hal_iommu_domain_id_t;

#define HAL_IOMMU_PROT_READ  (1 << 0)
#define HAL_IOMMU_PROT_WRITE (1 << 1)
#define HAL_IOMMU_PROT_EXEC  (1 << 2)
#define HAL_IOMMU_PROT_CACHE (1 << 3) // Memory is cache coherent

/**
 * Initializes the hardware IOMMU subsystem.
 *
 * @return 0 on success, negative error code on failure (e.g. absent).
 */
int hal_iommu_init(void);

/**
 * Creates a new protection domain.
 *
 * @param out_domain Pointer to store the new domain ID.
 * @return 0 on success, negative error code on failure.
 */
int hal_iommu_domain_alloc(hal_iommu_domain_id_t *out_domain);

/**
 * Frees a protection domain.
 *
 * @param domain The domain ID to free.
 * @return 0 on success, negative error code on failure.
 */
int hal_iommu_domain_free(hal_iommu_domain_id_t domain);

/**
 * Attaches a specific hardware device (e.g., via PCIe BDF or ACPI ID) to a domain.
 *
 * @param domain The domain ID.
 * @param device_id The hardware identifier of the device.
 * @return 0 on success, negative error code on failure.
 */
int hal_iommu_attach_device(hal_iommu_domain_id_t domain, uint32_t device_id);

/**
 * Maps a physical address to an IOVA within a specific protection domain.
 *
 * @param domain The domain ID.
 * @param iova The target IO Virtual Address.
 * @param paddr The source Physical Address.
 * @param size The size of the mapping.
 * @param prot Protection flags (HAL_IOMMU_PROT_*).
 * @return 0 on success, negative error code on failure.
 */
int hal_iommu_map(hal_iommu_domain_id_t domain, uint64_t iova, uint64_t paddr, size_t size, uint32_t prot);

/**
 * Unmaps an IOVA from a specific protection domain.
 *
 * @param domain The domain ID.
 * @param iova The IO Virtual Address to unmap.
 * @param size The size of the unmapping.
 * @return The size successfully unmapped, or a negative error code on failure.
 */
int hal_iommu_unmap(hal_iommu_domain_id_t domain, uint64_t iova, size_t size);

// Note: This API definition represents the future canonical model for DMA/IOMMU
// isolation and routing. Legacy platforms currently utilize `hal_iommu_ops_legacy_t`
// and are bridged internally.

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HAL_IOMMU_H */
