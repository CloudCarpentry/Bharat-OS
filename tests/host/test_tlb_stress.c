#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "../../kernel/include/hal/hal_tlb.h"
#include "../../kernel/include/mm/aspace.h"
#include "../../kernel/include/mm/mm_remote.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/urpc/urpc_bootstrap.h"
#include "../../services/core/subsysmgr/include/bharat/msg/transport.h"
#include "../../services/monitor/generated/bharat_monitor_v1_types.h"

// --- Mocking ---
hal_tlb_ops_t *active_hal_tlb;
static hal_tlb_ops_t mock_ops;

uint32_t hal_cpu_get_id(void) {
    return 0; // Simulate we are always core 0 for the initiator
}

void hal_core_poll_event(void) {
    __asm__ volatile("rep nop" ::: "memory");
}

int urpc_is_ready(uint32_t core) {
    return 1;
}

int urpc_bootstrap_send(uint32_t core, uint64_t msg) {
    return 0;
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    // We delay the ACK or drop it for stress testing
    mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[target_core];
    if (mailbox->valid) {
        // Randomly simulate a slow core or a missed tick by NOT acking immediately
        if (rand() % 10 != 0) { // 90% chance to ACK immediately
            mailbox->ack_seq = mailbox->msg.seq;
            mailbox->valid = 0;
        }
    }
}

// In a real environment `vmm_process_urpc_messages` would check mailboxes, we mock the fact that eventually delayed cores will ACK
void mock_background_ack_sweep() {
    for (int i=1; i<MAX_CPUS; i++) {
        mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[i];
        if (mailbox->valid && mailbox->req_seq != mailbox->ack_seq) {
             mailbox->ack_seq = mailbox->msg.seq;
             mailbox->valid = 0;
        }
    }
}

mm_mailbox_slot_t g_mm_mailboxes[64];

int bharat_monitor_v1_call_tlb_invalidate(
    bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, void* ctx) {
    return 0;
}

static void setup_test() {
    active_hal_tlb = &mock_ops;
    memset(g_cpu_locals, 0, sizeof(g_cpu_locals));
    memset(g_mm_mailboxes, 0, sizeof(g_mm_mailboxes));
    for (int i=0; i<64; i++) {
        spin_lock_init(&g_mm_mailboxes[i].lock);
    }
}

void test_tlb_stress_delayed_acks() {
    setup_test();
    address_space_t as = {0};
    as.object_id = 4;
    as.tlb_gen = 0;

    g_cpu_locals[0].current_as_id = 1;

    // Simulate 31 remote cores running AS 4
    for (int i=1; i<32; i++) {
        g_cpu_locals[i].current_as = &as;
        g_cpu_locals[i].current_as_id = 4;
        as.active_mask |= (1ULL << i);
    }

    // Launch a thread to occasionally ACK delayed messages
    pthread_t ack_thread;
    void *ack_runner(void *arg) {
        for (int i=0; i<100; i++) {
            usleep(100);
            mock_background_ack_sweep();
        }
        return NULL;
    }
    pthread_create(&ack_thread, NULL, ack_runner, NULL);

    for (int i=0; i<100; i++) {
        hal_tlb_invalidate_page(&as, 0x1000 + i * 4096);
    }

    pthread_join(ack_thread, NULL);
    assert(as.tlb_gen == 100);

    printf("test_tlb_stress_delayed_acks PASSED\n");
}

int main() {
    test_tlb_stress_delayed_acks();
    printf("All TLB stress tests passed.\n");
    return 0;
}
