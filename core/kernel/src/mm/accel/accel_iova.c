#include "../../include/mm/accel_mem.h"
#include "../../include/kernel/status.h"
#include "../../include/mm/iommu.h"
#include "../../include/mm/dma.h"
#include "../../include/mm/vm_mapping.h"
#include <stddef.h>

int accel_iova_bind(accel_buffer_t *buf, struct iommu_domain *domain) {
    if (!buf || !domain || buf->state < ACCEL_BUF_STATE_SG_BUILT) {
        return K_ERR_INVALID_ARG;
    }

    if (buf->state >= ACCEL_BUF_STATE_IOVA_BOUND) {
        return K_ERR_ALREADY_EXISTS;
    }

    accel_sg_entry_t *entry = buf->sg_table->head;
    uintptr_t iova_start = 0;

    uint64_t allocated_iova = 0;
    int ret = iova_alloc((iova_domain_t*)domain, buf->size, &allocated_iova);
    if (ret != K_OK) {
        return ret;
    }

    iova_start = allocated_iova;

    while (entry) {
        int map_ret = iommu_map(domain, iova_start, entry->phys_addr, entry->length,
                                VM_PROT_READ | VM_PROT_WRITE, 0);

        if (map_ret != K_OK) {
            uintptr_t unmap_iova = allocated_iova;
            accel_sg_entry_t *unmap_node = buf->sg_table->head;
            while (unmap_node != entry) {
                iommu_unmap(domain, unmap_iova, unmap_node->length);
                unmap_iova += unmap_node->length;
                unmap_node = unmap_node->next;
            }
            iova_free((iova_domain_t*)domain, allocated_iova, buf->size);
            return map_ret;
        }

        iova_start += entry->length;
        entry = entry->next;
    }

    buf->iova = allocated_iova;
    buf->iommu_domain = domain;
    buf->state = ACCEL_BUF_STATE_IOVA_BOUND;

    return K_OK;
}

int accel_iova_unbind(accel_buffer_t *buf) {
    if (!buf || buf->state < ACCEL_BUF_STATE_IOVA_BOUND) {
        return K_ERR_BAD_STATE;
    }

    if (!buf->iommu_domain) {
        return K_ERR_BAD_STATE;
    }

    accel_sg_entry_t *entry = buf->sg_table->head;
    uintptr_t current_iova = buf->iova;

    while (entry) {
        iommu_unmap(buf->iommu_domain, current_iova, entry->length);
        current_iova += entry->length;
        entry = entry->next;
    }

    iova_free((iova_domain_t*)buf->iommu_domain, buf->iova, buf->size);

    buf->iova = 0;
    buf->iommu_domain = NULL;
    buf->state = ACCEL_BUF_STATE_SG_BUILT;

    return K_OK;
}
