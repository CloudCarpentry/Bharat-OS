#include "../../include/mm.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"
#include "../../include/hal/mmu_ops.h"

// VM Fault Engine: Core page fault dispatcher and handler

typedef enum {
    FAULT_STATE_INIT = 0,
    FAULT_STATE_LOOKUP,
    FAULT_STATE_CHECK_PERM,
    FAULT_STATE_OBJECT_RESOLVE,
    FAULT_STATE_BACKEND_REPAIR,
    FAULT_STATE_TLB_SYNC,
    FAULT_STATE_SUCCESS,
    FAULT_STATE_ERROR
} fault_state_t;

typedef struct {
    address_space_t *aspace;
    virt_addr_t fault_addr;
    uint32_t fault_flags;

    vm_region_t *region;
    vm_object_t *object;
    phys_addr_t paddr;
    uint32_t page_flags;
    int error_code;
} fault_ctx_t;

int vm_handle_fault(address_space_t *aspace, virt_addr_t fault_addr, uint32_t fault_flags) {
    if (!aspace || !active_hal_pt) {
        return VM_FAULT_SIGSEGV;
    }

    fault_ctx_t ctx = {
        .aspace = aspace,
        .fault_addr = fault_addr,
        .fault_flags = fault_flags,
        .region = NULL,
        .object = NULL,
        .paddr = 0,
        .page_flags = 0,
        .error_code = 0
    };

    fault_state_t state = FAULT_STATE_INIT;

    while (state != FAULT_STATE_SUCCESS && state != FAULT_STATE_ERROR) {
        switch (state) {
            case FAULT_STATE_INIT:
                state = FAULT_STATE_LOOKUP;
                break;

            case FAULT_STATE_LOOKUP:
                ctx.region = aspace_lookup_region(ctx.aspace, ctx.fault_addr);
                if (!ctx.region) {
                    ctx.error_code = VM_FAULT_SIGSEGV;
                    state = FAULT_STATE_ERROR;
                } else {
                    state = FAULT_STATE_CHECK_PERM;
                }
                break;

            case FAULT_STATE_CHECK_PERM:
                if ((ctx.fault_flags & CAP_RIGHT_WRITE) && !(ctx.region->prot & CAP_RIGHT_WRITE)) {
                    ctx.error_code = -2; // Permission fault
                    state = FAULT_STATE_ERROR;
                } else if ((ctx.fault_flags & CAP_RIGHT_EXECUTE) && !(ctx.region->prot & CAP_RIGHT_EXECUTE)) {
                    ctx.error_code = -2; // Execute permission fault
                    state = FAULT_STATE_ERROR;
                } else {
                    state = FAULT_STATE_OBJECT_RESOLVE;
                }
                break;

            case FAULT_STATE_OBJECT_RESOLVE:
                ctx.object = ctx.region->object;
                if (!ctx.object || !ctx.object->ops || !ctx.object->ops->fault) {
                    ctx.error_code = VM_FAULT_SIGBUS;
                    state = FAULT_STATE_ERROR;
                } else {
                    int res = ctx.object->ops->fault(ctx.object, ctx.region, ctx.fault_addr, ctx.fault_flags, &ctx.paddr, &ctx.page_flags);
                    if (res != VM_FAULT_HANDLED) {
                        ctx.error_code = res;
                        state = FAULT_STATE_ERROR;
                    } else {
                        state = FAULT_STATE_BACKEND_REPAIR;
                    }
                }
                break;

            case FAULT_STATE_BACKEND_REPAIR: {
                uint32_t mmu_flags = MMU_USER; // Base common flag
                if (ctx.page_flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
                if (ctx.page_flags & CAP_RIGHT_EXECUTE) mmu_flags |= MMU_EXEC;
                // If hal_pt mapped it to generic flags, map MMU_WRITE to HAL_PT_FLAG_WRITE etc
                uint32_t pt_flags = HAL_PT_FLAG_USER | HAL_PT_FLAG_READ;
                if (mmu_flags & MMU_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
                if (mmu_flags & MMU_EXEC) pt_flags |= HAL_PT_FLAG_EXEC;

                virt_addr_t aligned_vaddr = ctx.fault_addr & ~(PAGE_SIZE - 1U);
                int ret = active_hal_pt->map_page(ctx.aspace->root_pt, aligned_vaddr, ctx.paddr, pt_flags);
                if (ret == 0) {
                    state = FAULT_STATE_TLB_SYNC;
                } else {
                    ctx.error_code = ret;
                    state = FAULT_STATE_ERROR;
                }
                break;
            }

            case FAULT_STATE_TLB_SYNC: {
                virt_addr_t aligned_vaddr = ctx.fault_addr & ~(PAGE_SIZE - 1U);
                hal_tlb_invalidate_page(ctx.aspace, aligned_vaddr);
                state = FAULT_STATE_SUCCESS;
                break;
            }

            default:
                ctx.error_code = -1;
                state = FAULT_STATE_ERROR;
                break;
        }
    }

    if (state == FAULT_STATE_SUCCESS) {
        return 0; // Success
    } else {
        return ctx.error_code;
    }
}
