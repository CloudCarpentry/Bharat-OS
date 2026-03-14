#include "mm/address_token.h"
#include "security/isolation.h"
#include "security/audit.h"
#include "hal/iommu.h"
#include "profile.h"
#include "device.h"

#define BHARAT_MAX_ISOLATED_PROCS 64U

typedef struct {
    uint8_t used;
    bharat_isolation_class_t iso_class;
    bharat_isolation_context_t ctx;
} bharat_isolation_slot_t;

static bharat_isolation_slot_t g_isolation_slots[BHARAT_MAX_ISOLATED_PROCS];

static bharat_isolation_slot_t* isolation_find(uint32_t process_id) {
    uint32_t i;
    for (i = 0; i < BHARAT_MAX_ISOLATED_PROCS; ++i) {
        if (g_isolation_slots[i].used && g_isolation_slots[i].ctx.process_id == process_id) {
            return &g_isolation_slots[i];
        }
    }
    return (bharat_isolation_slot_t*)0;
}

int bharat_isolation_init(void) {
    uint32_t i;
    for (i = 0; i < BHARAT_MAX_ISOLATED_PROCS; ++i) {
        g_isolation_slots[i].used = 0U;
    }
    return 0;
}

int bharat_isolation_bind_process(uint32_t process_id,
                                  uint32_t address_space_id,
                                  bharat_isolation_class_t iso_class,
                                  bharat_isolation_context_t* out_ctx) {
    uint32_t i;
    bharat_isolation_slot_t* slot = isolation_find(process_id);
    if (!slot) {
        for (i = 0; i < BHARAT_MAX_ISOLATED_PROCS; ++i) {
            if (!g_isolation_slots[i].used) {
                slot = &g_isolation_slots[i];
                slot->used = 1U;
                break;
            }
        }
    }

    if (!slot) {
        return -1;
    }

    slot->iso_class = iso_class;
    slot->ctx.process_id = process_id;
    slot->ctx.address_space_id = address_space_id;
    slot->ctx.sandbox_flags = 0U;
    slot->ctx.service_isolated = 0U;

    if (out_ctx) {
        *out_ctx = slot->ctx;
    }

    return 0;
}

int bharat_isolation_apply_sandbox(const bharat_sandbox_policy_t* policy,
                                   bharat_isolation_context_t* ctx) {
    if (!policy || !ctx) {
        return -1;
    }

    ctx->sandbox_flags = BHARAT_SANDBOX_FLAG_SYSCALL_FILTER;
    if (policy->allowed_mmio_regions == 0U) {
        ctx->sandbox_flags |= BHARAT_SANDBOX_FLAG_MMIO_RESTRICTED;
    }

    if (policy->process_id == ctx->process_id) {
        ctx->sandbox_flags |= BHARAT_SANDBOX_FLAG_SERVICE_CONTAINED;
        ctx->service_isolated = 1U;
    }

    (void)bharat_audit_record(BHARAT_AUDIT_EVENT_SANDBOX_APPLY,
                              ctx->process_id,
                              0,
                              ctx->sandbox_flags,
                              policy->allowed_syscall_mask);

    return 0;
}

struct bharat_iommu_domain {
    bharat_iommu_domain_config_t config;
    uint32_t id;
    uint8_t used;
    hal_iommu_domain_t* hal_domain;
};

struct bharat_iommu_group {
    uint32_t id;
    bharat_iommu_domain_t* domain;
    uint8_t used;
    hal_iommu_group_t* hal_group;
};

#define BHARAT_MAX_IOMMU_DOMAINS 32U
#define BHARAT_MAX_IOMMU_GROUPS  64U

static bharat_iommu_domain_t g_iommu_domains[BHARAT_MAX_IOMMU_DOMAINS];
static bharat_iommu_group_t g_iommu_groups[BHARAT_MAX_IOMMU_GROUPS];

int bharat_iommu_domain_create(const bharat_iommu_domain_config_t* cfg,
                               bharat_iommu_domain_t** out_domain) {
    uint32_t i;
    if (!cfg || !out_domain) {
        return -1;
    }

    hal_iommu_domain_t* hal_domain = NULL;
    int ret = hal_iommu_domain_create(cfg, &hal_domain);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < BHARAT_MAX_IOMMU_DOMAINS; ++i) {
        if (!g_iommu_domains[i].used) {
            g_iommu_domains[i].used = 1;
            g_iommu_domains[i].id = i;
            g_iommu_domains[i].config = *cfg;
            g_iommu_domains[i].hal_domain = hal_domain;
            *out_domain = &g_iommu_domains[i];
            return 0;
        }
    }

    hal_iommu_domain_destroy(hal_domain);
    return -1;
}

int bharat_iommu_domain_destroy(bharat_iommu_domain_t* domain) {
    if (!domain || !domain->used) {
        return -1;
    }

    if (domain->hal_domain) {
        hal_iommu_domain_destroy(domain->hal_domain);
        domain->hal_domain = NULL;
    }

    domain->used = 0;
    return 0;
}

int bharat_iommu_group_attach(bharat_iommu_group_t* group,
                              bharat_iommu_domain_t* domain) {
    if (!group || !domain) {
        return -1;
    }

    if (group->domain) {
        return -1; // Already attached
    }

    int ret = hal_iommu_group_attach(group->hal_group, domain->hal_domain);
    if (ret < 0) {
        return ret;
    }

    group->domain = domain;
    return 0;
}

int bharat_iommu_group_detach(bharat_iommu_group_t* group) {
    if (!group) {
        return -1;
    }

    if (!group->domain) {
        return 0; // Not attached
    }

    int ret = hal_iommu_group_detach(group->hal_group);
    if (ret < 0) {
        return ret;
    }

    group->domain = (bharat_iommu_domain_t*)0;
    return 0;
}

int bharat_iommu_map(bharat_iommu_domain_t* domain,
                     uint64_t iova,
                     uint64_t phys,
                     size_t size,
                     uint64_t prot_flags) {
    if (!domain || !domain->used) {
        return -1;
    }

    return hal_iommu_map(domain->hal_domain, iova, phys, size, prot_flags);
}

int bharat_iommu_unmap(bharat_iommu_domain_t* domain,
                       uint64_t iova,
                       size_t size) {
    if (!domain || !domain->used) {
        return -1;
    }

    return hal_iommu_unmap(domain->hal_domain, iova, size);
}

int bharat_iommu_block_device(bharat_device_t* dev) {
    int ret = hal_iommu_block_device(dev);
    if (ret == -1) { // -ENOSYS means no IOMMU
        SystemProfile profile = get_system_profile();
        MemoryModel model = get_memory_model();

        if (model != MEM_MODEL_MMU) {
            // MCU / MPU profile: Explicitly skip DMA-isolation promises, don't fail device
            return 0;
        }

        if (profile == PROFILE_TIER_C) { // Datacenter/High-isolation
            // Block DMA-capable devices if no IOMMU is present
            return -1;
        } else if (profile == PROFILE_TIER_B) { // Edge/Appliance
            // Allow trusted-only operation
            return 0;
        }
    }
    return ret;
}

int bharat_iommu_get_group(bharat_device_t* dev,
                           bharat_iommu_group_t** out_group) {
    if (!out_group) return -1;

    hal_iommu_group_t* hal_group = NULL;
    int ret = hal_iommu_get_group(dev, &hal_group);
    if (ret < 0) {
        return ret;
    }

    for (uint32_t i = 0; i < BHARAT_MAX_IOMMU_GROUPS; ++i) {
        if (!g_iommu_groups[i].used) {
            g_iommu_groups[i].used = 1;
            g_iommu_groups[i].id = i;
            g_iommu_groups[i].hal_group = hal_group;
            *out_group = &g_iommu_groups[i];
            return 0;
        }
    }

    return -1;
}

int bharat_addr_token_validate(const bharat_addr_token_t* token,
                               uint64_t addr,
                               uint64_t size,
                               uint32_t required_flags) {
    if (!token) {
        return -1;
    }

    if (!(token->flags & ADDR_TOKEN_FLAG_VALID)) {
        return -2;
    }

    if (token->flags & ADDR_TOKEN_FLAG_REVOKED) {
        return -3;
    }

    if (token->flags & ADDR_TOKEN_FLAG_EXPIRED) {
        return -4;
    }

    if ((token->flags & required_flags) != required_flags) {
        return -5;
    }

    if (addr < token->resource_base || (addr + size) > (token->resource_base + token->resource_size)) {
        return -6;
    }

    return 0;
}

void bharat_addr_token_revoke(bharat_addr_token_t* token) {
    if (token) {
        token->flags |= ADDR_TOKEN_FLAG_REVOKED;
        token->flags &= ~ADDR_TOKEN_FLAG_VALID;
    }
}
