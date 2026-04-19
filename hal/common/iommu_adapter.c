#include "hal/hal_iommu.h"
#include "mm/iommu.h"
#include <stddef.h>

/* The adapter layer to route new hal_iommu_ops_t calls into old hal_iommu_ops_legacy_t
 * until all platforms migrate over to implementing hal_iommu_ops_t natively.
 */

static hal_iommu_ops_legacy_t *g_legacy_ops = NULL;

void hal_iommu_register_ops(hal_iommu_ops_legacy_t *ops) {
    g_legacy_ops = ops;
    extern const hal_iommu_ops_t adapter_ops;
    hal_iommu_set_ops(&adapter_ops);
}

static int adapter_init(void) {
    if (g_legacy_ops && g_legacy_ops->init) {
        return g_legacy_ops->init();
    }
    return -1; // -ENOSYS
}

static iommu_state_t adapter_query_state(void) {
    if (g_legacy_ops) return IOMMU_STATE_ENABLED;
    return IOMMU_STATE_NONE;
}

static int adapter_domain_create(iommu_domain_t *dom) {
    // The legacy API expects a config and outputs a domain pointer.
    // Since we are adapting from new to old, we might just call it with NULL cfg
    // and store the pointer inside the new domain struct if possible.
    hal_iommu_domain_t *legacy_dom = NULL;
    if (g_legacy_ops && g_legacy_ops->domain_create) {
        int ret = g_legacy_ops->domain_create(NULL, &legacy_dom);
        if (ret == 0 && dom) {
            dom->hal_priv = (void*)legacy_dom;
        }
        return ret;
    }
    return -1;
}

static void adapter_domain_destroy(iommu_domain_t *dom) {
    if (g_legacy_ops && g_legacy_ops->domain_destroy && dom) {
        g_legacy_ops->domain_destroy((hal_iommu_domain_t*)dom->hal_priv);
    }
}

static int adapter_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
                size_t len, uint64_t prot, uint64_t flags) {
    if (g_legacy_ops && g_legacy_ops->map && dom) {
        return g_legacy_ops->map((hal_iommu_domain_t*)dom->hal_priv, iova, pa, len, prot);
    }
    return -1;
}

static int adapter_unmap(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    if (g_legacy_ops && g_legacy_ops->unmap && dom) {
        return g_legacy_ops->unmap((hal_iommu_domain_t*)dom->hal_priv, iova, len);
    }
    return -1;
}

static int adapter_attach_device(iommu_domain_t *dom, iommu_device_t *dev) {
    // Legacy relies on groups, new API does not immediately. Return unsupported for now.
    return -1;
}

static int adapter_detach_device(iommu_device_t *dev) {
    return -1;
}

static int adapter_invalidate_domain(iommu_domain_t *dom) {
    return -1;
}

static int adapter_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    return -1;
}

static int adapter_query_device_caps(iommu_device_t *dev, iommu_device_caps_t *caps) {
    return -1;
}

const hal_iommu_ops_t adapter_ops = {
    .init = adapter_init,
    .query_state = adapter_query_state,
    .domain_create = adapter_domain_create,
    .domain_destroy = adapter_domain_destroy,
    .attach_device = adapter_attach_device,
    .detach_device = adapter_detach_device,
    .map = adapter_map,
    .unmap = adapter_unmap,
    .invalidate_domain = adapter_invalidate_domain,
    .invalidate_range = adapter_invalidate_range,
    .query_device_caps = adapter_query_device_caps
};

// hal_iommu_get_ops is already defined in kernel/src/mm/iommu/iommu.c

// These are still needed by the kernel because some mm/iommu code might call them directly
int hal_iommu_init(void) { return adapter_init(); }
int hal_iommu_domain_create(const bharat_iommu_domain_config_t *cfg, hal_iommu_domain_t **out) {
    if (g_legacy_ops && g_legacy_ops->domain_create) {
        return g_legacy_ops->domain_create(cfg, out);
    }
    return -1;
}
int hal_iommu_domain_destroy(hal_iommu_domain_t *domain) {
    if (g_legacy_ops && g_legacy_ops->domain_destroy) {
        return g_legacy_ops->domain_destroy(domain);
    }
    return -1;
}
int hal_iommu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    if (g_legacy_ops && g_legacy_ops->group_attach) {
        return g_legacy_ops->group_attach(group, domain);
    }
    return -1;
}
int hal_iommu_group_detach(hal_iommu_group_t *group) {
    if (g_legacy_ops && g_legacy_ops->group_detach) {
        return g_legacy_ops->group_detach(group);
    }
    return -1;
}
int hal_iommu_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    if (g_legacy_ops && g_legacy_ops->map) {
        return g_legacy_ops->map(domain, iova, phys, size, prot);
    }
    return -1;
}
int hal_iommu_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) {
    if (g_legacy_ops && g_legacy_ops->unmap) {
        return g_legacy_ops->unmap(domain, iova, size);
    }
    return -1;
}
int hal_iommu_block_device(bharat_device_t *dev) {
    if (g_legacy_ops && g_legacy_ops->block_device) {
        return g_legacy_ops->block_device(dev);
    }
    return -1;
}
int hal_iommu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    if (g_legacy_ops && g_legacy_ops->get_group) {
        return g_legacy_ops->get_group(dev, out_group);
    }
    return -1;
}
