#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"
#include <stddef.h>

hal_pt_ops_t *active_hal_pt = NULL;
hal_tlb_ops_t *active_hal_tlb = NULL;

#if defined(__x86_64__) || defined(_M_X64)
extern hal_pt_ops_t x86_hal_pt_ops;
extern hal_tlb_ops_t x86_hal_tlb_ops;
#elif defined(__aarch64__) || defined(_M_ARM64)
extern hal_pt_ops_t arm64_hal_pt_ops;
extern hal_tlb_ops_t arm64_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 64
extern hal_pt_ops_t riscv64_hal_pt_ops;
extern hal_tlb_ops_t riscv64_hal_tlb_ops;
#endif

void hal_pt_init(void) {
#if defined(__x86_64__) || defined(_M_X64)
    active_hal_pt = &x86_hal_pt_ops;
    active_hal_tlb = &x86_hal_tlb_ops;
#elif defined(__aarch64__) || defined(_M_ARM64)
    active_hal_pt = &arm64_hal_pt_ops;
    active_hal_tlb = &arm64_hal_tlb_ops;
#elif defined(__riscv) && __riscv_xlen == 64
    active_hal_pt = &riscv64_hal_pt_ops;
    active_hal_tlb = &riscv64_hal_tlb_ops;
#endif
}

void hal_tlb_init(void) {
    // Already set in hal_pt_init for simplicity
}
