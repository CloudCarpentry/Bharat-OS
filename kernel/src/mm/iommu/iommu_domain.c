#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"
#include "bharat/urpc.h"
#include "bharat/cpu_local.h"
#include "hal/hal.h"
#include "atomic.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

// Global structure for passing IOMMU invalidation arguments across cores via uRPC.
typedef struct {
    iommu_domain_t *dom;
    uintptr_t iova;
    size_t len;
    bool is_range;
    atomic_t acks_needed;
} iommu_inval_req_t;

static iommu_inval_req_t g_iommu_invals[MAX_CPUS];

void iommu_handle_inval_req(uint64_t payload, uint32_t source_core) {
    uint32_t req_core = (uint32_t)payload;
    if (req_core >= MAX_CPUS) return;

    iommu_inval_req_t *req = &g_iommu_invals[req_core];
    if (req->dom) {
        const hal_iommu_ops_t *ops = hal_iommu_get_ops();
        if (req->is_range) {
            if (ops && ops->invalidate_range) {
                ops->invalidate_range(req->dom, req->iova, req->len);
            }
        } else {
            if (ops && ops->invalidate_domain) {
                ops->invalidate_domain(req->dom);
            }
        }
    }

    urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_IOMMU_INVAL_ACK, payload));
}

void iommu_handle_inval_ack(uint64_t payload) {
    uint32_t req_core = (uint32_t)payload;
    if (req_core < MAX_CPUS) {
        atomic_sub(&g_iommu_invals[req_core].acks_needed, 1);
    }
}

iommu_domain_t *iommu_domain_create(uint32_t flags) {
    if (!iommu_available()) return NULL;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->domain_create) {
        iommu_domain_t *dom = kmalloc(sizeof(iommu_domain_t));
        if (dom) {
            dom->flags = flags;
            dom->hal_priv = NULL;
            int ret = ops->domain_create(dom);
            if (ret == 0) return dom;
            kfree(dom);
        }
    }
    return NULL;
}

void iommu_domain_destroy(iommu_domain_t *dom) {
    if (!dom || !iommu_available()) return;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->domain_destroy) {
        ops->domain_destroy(dom);
    }
    kfree(dom);
}

int iommu_invalidate_domain(iommu_domain_t *dom) {
    if (!dom || !iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    int ret = -1;
    if (ops && ops->invalidate_domain) {
        ret = ops->invalidate_domain(dom);
    }

    // Broadcast to other cores
    uint32_t current_core = hal_cpu_get_id();
    iommu_inval_req_t *req = &g_iommu_invals[current_core];
    req->dom = dom;
    req->is_range = false;
    atomic_set(&req->acks_needed, 0);

    for (uint32_t c = 0; c < MAX_CPUS; c++) {
        if (c != current_core && urpc_channel_get_state(c) == URPC_CHANNEL_BOUND) {
            atomic_add(&req->acks_needed, 1);
            urpc_bootstrap_send(c, urpc_pack_msg(URPC_IOMMU_INVAL, (uint64_t)current_core));
        }
    }

    while (atomic_get(&req->acks_needed) > 0) {
        extern void arch_cpu_relax(void);
        arch_cpu_relax();
        extern void vmm_process_urpc_messages(void);
        vmm_process_urpc_messages();
    }

    return ret;
}

int iommu_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    if (!dom || !iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    int ret = -1;
    if (ops && ops->invalidate_range) {
        ret = ops->invalidate_range(dom, iova, len);
    }

    // Broadcast to other cores
    uint32_t current_core = hal_cpu_get_id();
    iommu_inval_req_t *req = &g_iommu_invals[current_core];
    req->dom = dom;
    req->iova = iova;
    req->len = len;
    req->is_range = true;
    atomic_set(&req->acks_needed, 0);

    for (uint32_t c = 0; c < MAX_CPUS; c++) {
        if (c != current_core && urpc_channel_get_state(c) == URPC_CHANNEL_BOUND) {
            atomic_add(&req->acks_needed, 1);
            urpc_bootstrap_send(c, urpc_pack_msg(URPC_IOMMU_INVAL, (uint64_t)current_core));
        }
    }

    while (atomic_get(&req->acks_needed) > 0) {
        extern void arch_cpu_relax(void);
        arch_cpu_relax();
        extern void vmm_process_urpc_messages(void);
        vmm_process_urpc_messages();
    }

    return ret;
}
