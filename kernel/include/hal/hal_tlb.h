#ifndef BHARAT_HAL_HAL_TLB_H
#define BHARAT_HAL_HAL_TLB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int hal_tlb_invalidate_page_local(uint64_t virt_addr);
int hal_tlb_invalidate_range_local(uint64_t virt_addr, uint64_t length);
int hal_tlb_invalidate_asid_local(uint16_t asid);
int hal_tlb_invalidate_global(void);
int hal_tlb_shootdown_remote(uint64_t virt_addr, uint64_t length, uint16_t asid);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_HAL_TLB_H
