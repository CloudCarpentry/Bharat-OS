#include "mm/address_token.h"
#include "security/isolation.h"
#include "security/audit.h"

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

int bharat_isolation_iommu_attach(uint32_t device_id, uint32_t process_id) {
    (void)device_id;
    (void)process_id;
    /* TODO: Wire into architecture IOMMU driver when available. */
    return 0;
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
