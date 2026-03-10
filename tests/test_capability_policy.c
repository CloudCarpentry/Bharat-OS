#include "../kernel/include/advanced/ai_sched.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kernel/include/mm.h"
#include "../kernel/include/advanced/formal_verif.h"

void ipc_async_check_timeouts(uint64_t current_ticks) {
    (void)current_ticks;
}

int bharat_addr_token_validate(const bharat_addr_token_t* token,
                               uint64_t request_paddr,
                               uint64_t request_size,
                               uint32_t request_perms) {
    (void)token; (void)request_paddr; (void)request_size; (void)request_perms;
    return 0;
}

// Stubs for linking
int mm_pmm_init(uint32_t magic, void *memory_map) { return 0; }
void hal_tlb_flush(unsigned long long vaddr) { (void)vaddr; }
void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) { }


int main(void) {
    assert(vmm_init() == 0);

    capability_t valid_npu_cap = { .rights_mask = CAP_RIGHT_DEVICE_NPU };
    capability_t invalid_npu_cap = { .rights_mask = CAP_RIGHT_READ };

    // Valid mapping
    assert(vmm_map_device_mmio(0x3000U, 0x4000U, &valid_npu_cap, 1) == 0);

    // Invalid mapping should return -3 (ERR_CAP_DENIED)
    assert(vmm_map_device_mmio(0x5000U, 0x6000U, &invalid_npu_cap, 1) == -3);

    // Null capability should also be denied
    assert(vmm_map_device_mmio(0x7000U, 0x8000U, NULL, 1) == -3);

    printf("Capability policy tests passed.\n");
    return 0;
}

