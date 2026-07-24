#include <mm/tlb.h>
#include <mm/tlb_internal.h>
#include <mm/aspace.h>
#include <mm/mm_remote.h>
#include <hal/hal_tlb.h>
#include <kernel/status.h>
#include <arch/arch_caps.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mocks for shootdown.c integration
typedef struct {
    uint64_t aspace_id;
    uint64_t va_start;
    uint64_t length;
    uint32_t type;
    uint32_t generation;
} bharat_monitor_v1_TlbInvalidateReq_t;

typedef struct {
    uint32_t status;
} bharat_monitor_v1_TlbInvalidateResp_t;

typedef struct {
    int dummy;
    struct {
        int (*send)(void* t, const void* buf, size_t len);
        int (*recv)(void* t, void* buf, size_t max, size_t* rx_len);
        int (*poll)(void* t, int timeout);
    }* ops;
} bharat_transport_t;

// Mocks
static bool g_panic_reached = false;
uint32_t hal_cpu_get_id(void) { return 0; }
void arch_cpu_relax(void) {}
void kernel_panic(const char* m) {
    printf("PANIC (Expected): %s\n", m);
    g_panic_reached = true;
}
hal_tlb_ops_t *active_hal_tlb = (hal_tlb_ops_t*)1;

arch_caps_t arch_get_caps(void) {
    arch_caps_t c = {ARCH_CAP_BIT(ARCH_CAP_SMP)};
    return c;
}

const hal_tlb_caps_t *hal_tlb_caps(void) { return NULL; }
int tlb_invalidate_local(vm_aspace_t *as, uintptr_t va, size_t len, tlb_inv_kind_t kind) { return 0; }
void cap_handle_delegate_req(uint64_t p, uint32_t s) {}
void cap_handle_delegate_ack(uint64_t p) {}
void cap_handle_revoke_req(uint64_t p, uint32_t s) {}
void cap_handle_revoke_ack(uint64_t p) {}
int urpc_bootstrap_recv(int c, uint64_t *m) { return -1; }

// tlb_pending mocks
int tlb_pending_alloc(uint64_t aspace_id, uint64_t target_mask, uint32_t* out_reqid) {
    *out_reqid = 123;
    return 0;
}
static bool g_simulate_tlb_timeout = false;
bool tlb_pending_is_complete(uint32_t current_core, int slot) {
    if (g_simulate_tlb_timeout) return false;
    return true;
}
void tlb_pending_free(uint32_t current_core, int slot) {}
typedef struct { uint32_t fallback_count; } tlb_pending_stats_t;
tlb_pending_stats_t mock_stats;
tlb_pending_stats_t* tlb_pending_get_stats(uint32_t core_id) { return &mock_stats; }
uint32_t tlb_reqid_encode(uint32_t c, uint32_t s, uint32_t g) { return 0; }
void tlb_pending_ack(uint32_t req_id, uint32_t acking_core) {}

// aspace mocks
bool aspace_is_valid_for_tlb(address_space_t *aspace) { return true; }
uint64_t aspace_get_active_mask(address_space_t *aspace) { return aspace->active_mask; }
uint64_t aspace_next_tlb_generation(address_space_t *aspace) { return ++aspace->tlb_gen; }

// transport mocks
bharat_transport_t* transport_for_core(int core) { return NULL; }
int bharat_monitor_v1_call_tlb_invalidate(bharat_transport_t* t, int dst, const bharat_monitor_v1_TlbInvalidateReq_t* req, void* ctx) { return 0; }

// hal_ipi mocks
void hal_ipi_send(uint32_t core, uint32_t ipi) {}

// global mocks
mm_mailbox_slot_t g_mm_mailboxes[64];
tlb_cpu_state_t g_tlb_cpu_state[32];
cpu_local_t g_cpu_locals[32];

void test_tlb_shootdown_contract(void) {
    printf("Running test_tlb_shootdown_contract...\n");
    address_space_t as;
    as.object_id = 1;
    as.active_mask = 0;
    as.tlb_gen = 1;

    // No active targets, should return success immediately
    assert(tlb_invalidate_remote(&as, 0x1000, 0x1000, TLB_INV_PAGE) == 0);
}

void test_tlb_shootdown_timeout(void) {
    printf("Running test_tlb_shootdown_timeout...\n");
    address_space_t as;
    as.object_id = 1;
    as.active_mask = (1ULL << 1); // Target CPU 1
    as.tlb_gen = 1;

    g_simulate_tlb_timeout = true;

    // Expect failure with TLB_FAIL_RETURN_ERROR
    int ret = tlb_invalidate_remote_ex(&as, 0x1000, 0x1000, TLB_INV_PAGE, TLB_FAIL_RETURN_ERROR);
    assert(ret == -1);
    assert(g_panic_reached == false);

    // Expect panic with TLB_FAIL_KERNEL_PANIC
    g_panic_reached = false;
    ret = tlb_invalidate_remote_ex(&as, 0x1000, 0x1000, TLB_INV_PAGE, TLB_FAIL_KERNEL_PANIC);
    assert(g_panic_reached == true);
}

int main(void) {
    test_tlb_shootdown_contract();
    test_tlb_shootdown_timeout();
    printf("All tlb_shootdown host tests passed!\n");
    return 0;
}
