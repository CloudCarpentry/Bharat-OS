#ifndef BHARAT_MM_TLB_H
#define BHARAT_MM_TLB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <kernel/status.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declare to break dependency cycle
struct vm_address_space;
typedef struct vm_address_space address_space_t;
typedef struct vm_address_space vm_aspace_t;

typedef enum {
    TLB_INV_PAGE,
    TLB_INV_RANGE,
    TLB_INV_ASPACE,
    TLB_INV_FULL,
} tlb_inv_kind_t;

typedef enum {
    TLB_FAIL_RETURN_ERROR = 0,
    TLB_FAIL_ISOLATE_ASPACE,
    TLB_FAIL_KERNEL_PANIC
} tlb_failure_policy_t;

typedef struct {
    uint32_t request_id;
    uint64_t aspace_id;
    uint32_t tlb_generation;
    uint64_t target_mask;
    uint64_t ack_mask;
    uint64_t missing_mask;
    uint64_t dispatch_failure_mask;
    uint32_t retry_count;
    uint64_t start_tick;
    uint64_t elapsed_ticks;
    int final_status;
    bool valid;
} tlb_failure_snapshot_t;

int tlb_init(void);

int tlb_invalidate_local(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);
int tlb_invalidate_remote(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);
int tlb_invalidate_remote_ex(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind, tlb_failure_policy_t failure_policy);
int tlb_invalidate_all(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);
kstatus_t tlb_invalidate_all_ex(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind, tlb_failure_policy_t failure_policy);

kstatus_t tlb_diag_get_last_failure(uint32_t cpu, tlb_failure_snapshot_t *out);

// Backward compatibility or direct call for one page shootdown
void tlb_shootdown(vm_aspace_t *as, uint64_t vaddr);

// Exposed for stats printing
void tlb_dump_stats(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_TLB_H
