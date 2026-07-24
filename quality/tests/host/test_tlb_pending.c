#include <mm/tlb_internal.h>
#include "tlb_pending.h"
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mocks
uint32_t hal_cpu_get_id(void) { return 0; }

void test_tlb_pending_basic(void) {
    printf("Running test_tlb_pending_basic...\n");
    tlb_pending_init();

    uint32_t reqid = 0;
    int slot = tlb_pending_alloc(0x1, 0x6, &reqid); // target cores 1 and 2
    assert(slot >= 0);
    assert(reqid != 0);

    assert(tlb_pending_is_complete(0, slot) == false);

    // Ack from core 1
    tlb_pending_ack(reqid, 1);
    assert(tlb_pending_is_complete(0, slot) == false);

    // Ack from core 2
    tlb_pending_ack(reqid, 2);
    assert(tlb_pending_is_complete(0, slot) == true);

    tlb_pending_free(0, slot);
}

void test_tlb_pending_stale_ack(void) {
    printf("Running test_tlb_pending_stale_ack...\n");
    tlb_pending_init();

    uint32_t reqid = 0;
    int slot = tlb_pending_alloc(0x1, 0x2, &reqid);

    tlb_pending_free(0, slot);

    // Ack for a slot that was just freed
    tlb_pending_ack(reqid, 1);

    tlb_pending_stats_t* stats = tlb_pending_get_stats(0);
    assert(stats->stale_acks == 1);
}

int main(void) {
    test_tlb_pending_basic();
    test_tlb_pending_stale_ack();
    printf("All tlb_pending host tests passed!\n");
    return 0;
}
