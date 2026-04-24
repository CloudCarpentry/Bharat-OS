#ifndef BHARAT_MM_TLB_H
#define BHARAT_MM_TLB_H

#include <stdint.h>
#include <stddef.h>

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

int tlb_init(void);

int tlb_invalidate_local(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);
int tlb_invalidate_remote(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);
int tlb_invalidate_all(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind);

// Backward compatibility or direct call for one page shootdown
void tlb_shootdown(vm_aspace_t *as, uint64_t vaddr);

// Exposed for stats printing
void tlb_dump_stats(void);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_TLB_H
