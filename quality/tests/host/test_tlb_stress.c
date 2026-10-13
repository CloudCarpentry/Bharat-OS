#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <hal/hal_tlb.h>
#include <mm/aspace.h>
#include <mm/mm_remote.h>
#include <bharat/cpu_local.h>
#include <hal/hal.h>
#include <urpc/urpc_bootstrap.h>
#include <arch/arch_caps.h>
#include <mm/tlb_internal.h>
#include <kernel/status.h>

typedef struct {
    uint64_t aspace_id;
    uint64_t va_start;
    uint64_t length;
    uint32_t type;
    uint32_t generation;
} bharat_monitor_v1_TlbInvalidateReq_t;

typedef void* bharat_transport_t;

// --- Mocking ---
hal_tlb_ops_t *active_hal_tlb;
static hal_tlb_ops_t mock_ops;

static uint32_t g_current_cpu_id = 0;

uint32_t hal_cpu_get_id(void) {
    return g_current_cpu_id;
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

uint32_t g_active_core_count = 32;
tlb_cpu_state_t g_tlb_cpu_state[MAX_CPUS];
cpu_local_t g_cpu_locals[MAX_CPUS];

mem_model_t mem_model_get_current(void) { return MEM_MODEL_MMU_FULL; }

int bharat_monitor_v1_send_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, uint64_t reqid, void* ctx) {
    (void)t; (void)dst; (void)req; (void)reqid; (void)ctx;
    return 0;
}

bool hal_timer_monotonic_ns(uint64_t *out_ns) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (out_ns) *out_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    return true;
}

void arch_cpu_relax(void) {
    usleep(10);
}

arch_caps_t arch_get_caps(void) {
    arch_caps_t c = {ARCH_CAP_BIT(ARCH_CAP_SMP)};
    return c;
}

const hal_tlb_caps_t *hal_tlb_caps(void) {
    static hal_tlb_caps_t c = {
        .supports_page_flush = 1,
        .supports_range_flush = 1,
        .supports_aspace_flush = 1,
        .supports_all_flush = 1
    };
    return &c;
}

void console_log(int level, const char* fmt, ...) { (void)level; (void)fmt; }
void console_write_raw(const char* s, size_t len) { (void)s; (void)len; }
void kernel_panic(const char* m) { (void)m; }

bool aspace_is_valid_for_tlb(address_space_t *aspace) { return aspace != NULL; }
uint64_t aspace_get_active_mask(address_space_t *aspace) { return aspace ? aspace->active_mask : 0; }
uint64_t aspace_next_tlb_generation(address_space_t *aspace) { return aspace ? ++aspace->tlb_gen : 0; }
void aspace_mark_poisoned(address_space_t *aspace) { if (aspace) aspace->flags |= ASPACE_STATE_POISONED; }

bharat_transport_t* transport_for_core(int core) { (void)core; return NULL; }

void hal_ipi_send(uint32_t target_core, uint32_t ipi) {
    (void)ipi;
    if (target_core < 64) {
        mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[target_core];
        if (mailbox->valid) {
            if (rand() % 10 != 0) { // 90% chance to ACK immediately
                mailbox->ack_seq = mailbox->msg.seq;
                mailbox->valid = 0;
                extern void tlb_send_completion(uint32_t request_id, uint32_t origin_cpu, uint32_t status);
                uint32_t saved_cpu = g_current_cpu_id;
                g_current_cpu_id = target_core;
                tlb_send_completion(mailbox->msg.seq, mailbox->msg.sender_core, 0);
                g_current_cpu_id = saved_cpu;
            }
        }
    }
}

void cap_handle_delegate_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_delegate_ack(uint64_t p) { (void)p; }
void cap_handle_revoke_req(uint64_t p, uint32_t s) { (void)p; (void)s; }
void cap_handle_revoke_ack(uint64_t p) { (void)p; }
kstatus_t urpc_bootstrap_recv(uint32_t c, uint64_t *m) { (void)c; (void)m; return -1; }
mm_mailbox_slot_t g_mm_mailboxes[64];

void mock_background_ack_sweep(void) {
    for (int i = 1; i < MAX_CPUS; i++) {
        mm_mailbox_slot_t* mailbox = &g_mm_mailboxes[i];
        if (mailbox->valid && mailbox->req_seq != mailbox->ack_seq) {
             mailbox->ack_seq = mailbox->msg.seq;
             mailbox->valid = 0;
             extern void tlb_send_completion(uint32_t request_id, uint32_t origin_cpu, uint32_t status);
             uint32_t saved_cpu = g_current_cpu_id;
             g_current_cpu_id = (uint32_t)i;
             tlb_send_completion(mailbox->msg.seq, mailbox->msg.sender_core, 0);
             g_current_cpu_id = saved_cpu;
        }
    }
}

static void setup_test() {
    g_current_cpu_id = 0;
    active_hal_tlb = &mock_ops;
    memset(g_cpu_locals, 0, sizeof(g_cpu_locals));
    memset(g_mm_mailboxes, 0, sizeof(g_mm_mailboxes));
    for (int i=0; i<64; i++) {
        spin_lock_init(&g_mm_mailboxes[i].lock);
    }
}

static void *ack_runner(void *arg) {
    (void)arg;
    for (int i=0; i<100; i++) {
        usleep(100);
        mock_background_ack_sweep();
    }
    return NULL;
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
