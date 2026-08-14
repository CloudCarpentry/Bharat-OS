#define BHARAT_HOST_TEST 1
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "../../kernel/include/capability.h"
#include "../../kernel/include/cap_policy.h"
#include "../../kernel/include/bharat/cpu_local.h"
#include "../../kernel/include/hal/hal.h"
#include "../../kernel/include/hal/hal_timer.h"
#include "../../kernel/include/time/ktime.h"
#include "../../lib/cap/include/bharat/cap/cap_validate.h"
#include "../../lib/cap/include/bharat/cap/cap_authz.h"
#include "../../../core/kernel/src/cap/cap_internal.h"

#include <bharat/uapi/ipc/status.h>
#include <bharat/uapi/servicemgr/contract.h>
#include <bharat/uapi/process_manager/contract.h>
#include <bharat/uapi/namesvc/contract.h>
#include <bharat/urpc.h>

// VM manager opcode definitions
#define VM_OP_MAP 1
#define VM_OP_FAULT 5

typedef struct {
    uint64_t aspace_id;
    uint64_t vaddr;
    uint64_t size;
    uint32_t flags;
} vm_req_map_t;

#include <stdlib.h>

// --- Mock structures and globals ---
uint32_t g_active_core_count = 4;
static uint32_t g_current_cpu_id = 0;
uint64_t g_fake_ticks = 0;

void* kmalloc(size_t size) {
    return malloc(size);
}

void kfree(void* ptr) {
    free(ptr);
}

cpu_local_t g_cpu_locals[MAX_CPUS];
capability_table_t g_mock_cap_tables[MAX_CPUS];
bool g_pmm_initialized = true;

static bool g_panic_triggered = false;
static const char* g_panic_message = NULL;

void kernel_panic(const char* m) {
    g_panic_triggered = true;
    g_panic_message = m;
}

uint32_t hal_cpu_get_id(void) {
    return g_current_cpu_id;
}

uint64_t hal_timer_read_freq(void) {
    return 1000;
}

uint64_t hal_timer_monotonic_ticks(void) {
    return g_fake_ticks++;
}

bh_ktime_t bh_ktime_now(void) {
    return (bh_ktime_t)g_fake_ticks * 1000000ULL; // Convert fake ticks to ns
}

bh_kdeadline_t bh_deadline_after_ns(uint64_t duration_ns) {
    return bh_ktime_now() + duration_ns;
}

bool bh_deadline_expired(bh_kdeadline_t deadline) {
    return bh_ktime_now() >= deadline;
}

uint64_t bh_deadline_remaining_ns(bh_kdeadline_t deadline) {
    bh_ktime_t now = bh_ktime_now();
    return (now >= deadline) ? 0 : (deadline - now);
}

// uRPC mock state and buffers
static urpc_channel_state_t g_urpc_states[MAX_CPUS];
static uint64_t g_urpc_inbox[MAX_CPUS][64];
static int g_urpc_head[MAX_CPUS];
static int g_urpc_tail[MAX_CPUS];

urpc_channel_state_t urpc_channel_get_state(uint32_t core) {
    if (core >= MAX_CPUS) return URPC_CHANNEL_CLOSED;
    return g_urpc_states[core];
}

int urpc_bootstrap_send(uint32_t target_core, uint64_t raw_msg) {
    if (target_core >= MAX_CPUS) return -1;
    int next_tail = (g_urpc_tail[target_core] + 1) % 64;
    if (next_tail == g_urpc_head[target_core]) return -1; // Full
    g_urpc_inbox[target_core][g_urpc_tail[target_core]] = raw_msg;
    g_urpc_tail[target_core] = next_tail;
    return 0;
}

int urpc_bootstrap_recv(uint32_t c, uint64_t *m) {
    if (c >= MAX_CPUS) return -1;
    if (g_urpc_head[c] == g_urpc_tail[c]) return -1; // Empty
    if (m) *m = g_urpc_inbox[c][g_urpc_head[c]];
    g_urpc_head[c] = (g_urpc_head[c] + 1) % 64;
    return 0;
}

// Mock console logging
void console_log(int level, const char* fmt, ...) {
    (void)level; (void)fmt;
}

void vmm_process_urpc_messages(void) {
    uint32_t current_cpu = hal_cpu_get_id();
    uint64_t msg;
    if (urpc_bootstrap_recv(current_cpu, &msg) == 0) {
        urpc_msg_type_t type;
        uint64_t payload;
        urpc_unpack_msg(msg, &type, &payload);
        if (type == URPC_CAP_DELEGATE_REQ || type == URPC_CAP_REVOKE) {
            uint8_t origin_core = 0, slot = 0, op = 0;
            uint32_t gen = 0;
            cap_tx_unpack_req(payload, &origin_core, &slot, &gen, &op);
            cap_handle_tx_req(payload, origin_core);
        } else if (type == URPC_CAP_DELEGATE_ACK || type == URPC_CAP_REVOKE_ACK) {
            cap_handle_tx_ack(payload);
        }
    }
}

void arch_cpu_relax(void) {
    g_fake_ticks++;
    // Let all cores process messages cooperatively in single-threaded test harness
    uint32_t saved = g_current_cpu_id;
    for (uint32_t i = 0; i < g_active_core_count; i++) {
        g_current_cpu_id = i;
        vmm_process_urpc_messages();
    }
    g_current_cpu_id = saved;
}

// Setup a process structure
bh_process_t g_mock_processes[MAX_CPUS];
bh_thread_t g_mock_threads[MAX_CPUS];

void setup_host_test_environment(void) {
    g_current_cpu_id = 0;
    g_fake_ticks = 0;
    g_panic_triggered = false;
    g_panic_message = NULL;

    memset(g_cpu_locals, 0, sizeof(g_cpu_locals));
    memset(g_mock_cap_tables, 0, sizeof(g_mock_cap_tables));
    memset(g_urpc_states, 0, sizeof(g_urpc_states));
    memset(g_urpc_head, 0, sizeof(g_urpc_head));
    memset(g_urpc_tail, 0, sizeof(g_urpc_tail));
    memset(g_cap_cspace_registry, 0, sizeof(g_cap_cspace_registry));
    memset(g_cap_tx_table, 0, sizeof(g_cap_tx_table));

    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (i < g_active_core_count) {
            g_urpc_states[i] = URPC_CHANNEL_BOUND;
        } else {
            g_urpc_states[i] = URPC_CHANNEL_CLOSED;
        }

        g_mock_processes[i].process_id = 100 + i;
        g_mock_threads[i].process = &g_mock_processes[i];

        g_cpu_locals[i].cpu_id = i;
        g_cpu_locals[i].current = &g_mock_threads[i];

        // Initialize capability tables per core
        capability_table_t *table = &g_mock_cap_tables[i];
        memset(table, 0, sizeof(*table));
        spin_lock_init(&table->lock);
        bh_id_allocator_init(&table->id_allocator, table->id_bitmap, BHARAT_ARRAY_SIZE(table->entries));
        table->owner_pid = g_mock_processes[i].process_id;
        table->owner_core = (uint16_t)i;
        table->registry_slot = 0;
        table->cspace_id = cap_cspace_id_make(i, 0, 1);

        g_cap_cspace_registry[i][0].table = table;
        g_cap_cspace_registry[i][0].generation = 1;

        g_mock_processes[i].security_sandbox_ctx = table;

        spin_lock_init(&g_cap_tx_lock[i]);
    }

    // Register canonical authority resolver for host tests
    extern bharat_cap_status_t kernel_cap_authority_resolver(
        bharat_cap_handle_t handle,
        bharat_cap_object_type_t expected_object_type,
        uint64_t expected_object_id,
        uint64_t required_rights,
        const bharat_cap_scope_t *required_scope,
        bharat_cap_validation_result_t *out_result);

    bharat_cap_register_authority_resolver(kernel_cap_authority_resolver);
}

// --- Test Level 1: Capability Validation Unit Tests ---
void test_level1_validation(void) {
    printf("Running Test Level 1: Capability Validation Unit Tests...\n");
    fflush(NULL);
    setup_host_test_environment();

    capability_table_t *table = &g_mock_cap_tables[0];
    uint32_t cap_id = 0;

    // 1. Basic Grant and Lookup using valid memory rights
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x100000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP, &cap_id);
    assert(ret == 0);
    assert(bh_cap_is_valid_encoding(cap_id));

    capability_entry_t entry;
    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == 0);
    assert(entry.object_ref == 0x100000);

    uint32_t raw_cap = bh_cap_index(cap_id);
    ret = cap_table_lookup(table, raw_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -6);

    uint32_t stale_cap = cap_id + (1U << 16);
    ret = cap_table_lookup(table, stale_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -6);

    ret = cap_table_lookup(table, cap_id, CAP_TYPE_PROCESS, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -2);

    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &entry);
    assert(ret == -3);

    ret = cap_table_revoke(table, cap_id);
    assert(ret == 0);

    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -7 || ret == -4);

    printf("  -> Level 1 PASSED!\n");
}

// --- Test Level 2: Service Authorization Tests ---
void test_level2_service_authorization(void) {
    printf("Running Test Level 2: Service Authorization Tests...\n");
    setup_host_test_environment();

    capability_table_t *table = &g_mock_cap_tables[0];

    // 1. VM Manager Descriptors
    static const bharat_service_authz_desc_t vm_manager_authz_descs[] = {
        {
            .opcode = VM_OP_MAP,
            .object_type = BHARAT_CAP_OBJ_VM_SPACE,
            .required_rights = BHARAT_CAP_RIGHT_WRITE,
            .required_feature_cap = BHARAT_MEM_CAP_PAGE_MAP,
        }
    };

    uint32_t vm_cap = 0;
    cap_table_grant(table, CAP_TYPE_MEMORY, 42, CAP_RIGHT_MEMORY_UNMAP, &vm_cap);

    int32_t status = bharat_service_dispatch_authorize(
        0x00010001, VM_OP_MAP, vm_manager_authz_descs, 1, vm_cap, 42
    );
    assert(status == BHARAT_IPC_STATUS_OK);

    // Service authorization uses the canonical validator and rejects raw handles.
    status = bharat_service_dispatch_authorize(
        0x00010001, VM_OP_MAP, vm_manager_authz_descs, 1, bh_cap_index(vm_cap), 42
    );
    assert(status == BHARAT_IPC_STATUS_ERR_PERM);

    status = bharat_service_dispatch_authorize(
        0x00010001, VM_OP_MAP, vm_manager_authz_descs, 1, BHARAT_CAP_INVALID_HANDLE, 42
    );
    assert(status == BHARAT_IPC_STATUS_ERR_PERM); // Deny invalid

    // 2. Process Manager Descriptors
    static const bharat_service_authz_desc_t process_manager_authz_descs[] = {
        {
            .opcode = PM_OP_START,
            .object_type = BHARAT_CAP_OBJ_PROCESS,
            .required_rights = BHARAT_CAP_RIGHT_EXECUTE,
        }
    };

    uint32_t pm_cap = 0;
    cap_table_grant(table, CAP_TYPE_PROCESS, 101, CAP_RIGHT_RESOURCE_ALLOC, &pm_cap);

    status = bharat_service_dispatch_authorize(
        0x00010001, PM_OP_START, process_manager_authz_descs, 1, pm_cap, 101
    );
    assert(status == BHARAT_IPC_STATUS_OK);

    printf("  -> Level 2 PASSED!\n");
}

// --- Test Level 3: Distributed Transaction Tests ---
void test_level3_distributed_transactions(void) {
    printf("Running Test Level 3: Distributed Transaction Tests...\n");
    setup_host_test_environment();

    capability_table_t *src = &g_mock_cap_tables[1];
    capability_table_t *dst = &g_mock_cap_tables[2];

    // 1. Cross-core Delegation happy path (CPU 1 -> CPU 2)
    uint32_t src_cap = 0;
    cap_table_grant(src, CAP_TYPE_MEMORY, 0x5000, CAP_RIGHT_DELEGATE | CAP_RIGHT_MEMORY_UNMAP, &src_cap);

    g_current_cpu_id = 1;
    uint32_t delegated_cap = 0;
    int ret = cap_table_delegate(src, dst, src_cap, CAP_RIGHT_MEMORY_UNMAP, &delegated_cap);

    assert(ret == 0);
    assert(bh_cap_is_valid_encoding(delegated_cap));

    // Verify it exists in destination table
    capability_entry_t entry;
    ret = cap_table_lookup(dst, delegated_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_UNMAP, &entry);
    assert(ret == 0);
    assert(entry.object_ref == 0x5000);

    // Revoke from CPU 1 and verify child on CPU 2 is revoked
    g_current_cpu_id = 1;
    ret = cap_table_revoke(src, src_cap);
    assert(ret == 0);

    // Lookup on CPU 2 should now fail
    ret = cap_table_lookup(dst, delegated_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_UNMAP, &entry);
    assert(ret != 0);

    printf("  -> Level 3 PASSED!\n");
}

// --- Test Level 4: Multicore Integration Tests ---
void test_level4_multicore_integration(void) {
    printf("Running Test Level 4: Multicore Integration Tests (CPU 2 -> CPU 3)...\n");
    setup_host_test_environment();

    capability_table_t *src = &g_mock_cap_tables[2];
    capability_table_t *dst = &g_mock_cap_tables[3];

    uint32_t src_cap = 0;
    cap_table_grant(src, CAP_TYPE_MEMORY, 0x8000, CAP_RIGHT_DELEGATE | CAP_RIGHT_MEMORY_UNMAP, &src_cap);

    // Initiate from CPU 2
    g_current_cpu_id = 2;
    uint32_t delegated_cap = 0;
    int ret = cap_table_delegate(src, dst, src_cap, CAP_RIGHT_MEMORY_UNMAP, &delegated_cap);
    assert(ret == 0);
    assert(bh_cap_is_valid_encoding(delegated_cap));

    // Verify CPU 3 owns it
    g_current_cpu_id = 3;
    capability_entry_t entry;
    ret = cap_table_lookup(dst, delegated_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_UNMAP, &entry);
    assert(ret == 0);
    assert(entry.object_ref == 0x8000);

    printf("  -> Level 4 PASSED!\n");
}

// --- Test Level 5: Security Edge Cases & Invariants ---

// 1. Duplicate ACK handling (idempotency)
void test_level5_duplicate_ack(void) {
    printf("Running Test Level 5.1: Duplicate ACK Idempotence...\n");
    setup_host_test_environment();

    g_current_cpu_id = 1;
    uint8_t slot = 0;
    bh_cap_tx_entry_t *tx = cap_tx_alloc(1, BH_CAP_TX_OP_DELEGATE, &slot);
    assert(tx != NULL);
    tx->target_mask = (1U << 2);
    tx->ack_mask = 0;
    atomic_set(&tx->state, BH_CAP_TX_PUBLISHED);

    // Prepare ACK payload
    uint64_t ack_payload = cap_tx_pack_ack(1, slot, tx->generation, 2, 0);

    // First ACK
    cap_handle_tx_ack(ack_payload);
    assert((tx->ack_mask & (1U << 2)) != 0);

    // Duplicate ACK
    cap_handle_tx_ack(ack_payload);
    assert(tx->ack_mask == (1U << 2)); // Mask unchanged, no overflow or corrupted state

    cap_tx_release(1, slot);
    printf("  -> Duplicate ACK Idempotence PASSED!\n");
}

// 2. Late / Stale ACK from previous generation rejection
void test_level5_stale_ack_rejection(void) {
    printf("Running Test Level 5.2: Stale ACK Rejection...\n");
    setup_host_test_environment();

    g_current_cpu_id = 1;
    uint8_t slot = 0;

    // Transaction A
    bh_cap_tx_entry_t *tx_a = cap_tx_alloc(1, BH_CAP_TX_OP_DELEGATE, &slot);
    assert(tx_a != NULL);
    uint32_t gen_a = tx_a->generation;
    atomic_set(&tx_a->state, BH_CAP_TX_COMMITTED);
    cap_tx_release(1, slot);

    // Transaction B on same slot
    bh_cap_tx_entry_t *tx_b = cap_tx_alloc(1, BH_CAP_TX_OP_DELEGATE, &slot);
    assert(tx_b != NULL);
    assert(tx_b->generation != gen_a);
    tx_b->target_mask = (1U << 2);
    tx_b->ack_mask = 0;
    atomic_set(&tx_b->state, BH_CAP_TX_PUBLISHED);

    // Late ACK from transaction A arriving during transaction B
    uint64_t stale_ack_payload = cap_tx_pack_ack(1, slot, gen_a, 2, 0);
    cap_handle_tx_ack(stale_ack_payload);

    // Transaction B must NOT have received this stale ACK
    assert((tx_b->ack_mask & (1U << 2)) == 0);

    // Genuine ACK for transaction B arrives
    uint64_t valid_ack_payload = cap_tx_pack_ack(1, slot, tx_b->generation, 2, 0);
    cap_handle_tx_ack(valid_ack_payload);
    assert((tx_b->ack_mask & (1U << 2)) != 0);

    cap_tx_release(1, slot);
    printf("  -> Stale ACK Rejection PASSED!\n");
}

// 3. Timeout and Abortion on lost ACK
void test_level5_lost_ack_timeout(void) {
    printf("Running Test Level 5.3: Lost ACK Timeout...\n");
    setup_host_test_environment();

    capability_table_t *src = &g_mock_cap_tables[1];
    capability_table_t *dst = &g_mock_cap_tables[2];

    uint32_t src_cap = 0;
    cap_table_grant(src, CAP_TYPE_MEMORY, 0x9000, CAP_RIGHT_DELEGATE | CAP_RIGHT_MEMORY_UNMAP, &src_cap);

    // Disconnect destination core's reply channel to simulate lost ACK
    g_urpc_states[2] = URPC_CHANNEL_CLOSED;

    g_current_cpu_id = 1;
    uint32_t delegated_cap = 0;
    int ret = cap_table_delegate(src, dst, src_cap, CAP_RIGHT_MEMORY_UNMAP, &delegated_cap);
    assert(ret == -6); // Failed closed before send or timed out

    printf("  -> Lost ACK Timeout PASSED!\n");
}

// 4. Source commit failure & Transactional Rollback Guarantee
void test_level5_rollback_guarantee(void) {
    printf("Running Test Level 5.4: Transactional Rollback Guarantee...\n");
    setup_host_test_environment();

    capability_table_t *src = &g_mock_cap_tables[1];
    capability_table_t *dst = &g_mock_cap_tables[2];

    uint32_t src_cap = 0;
    cap_table_grant(src, CAP_TYPE_MEMORY, 0xA000, CAP_RIGHT_DELEGATE | CAP_RIGHT_MEMORY_UNMAP, &src_cap);

    // Invalidate source capability right before delegation is executed by invalidating generation
    uint32_t slot_idx = bh_cap_index(src_cap) - 1;
    src->entries[slot_idx].state = CAP_STATE_FREE; // Invalidate source

    g_current_cpu_id = 1;
    uint32_t delegated_cap = 0;
    int ret = cap_table_delegate(src, dst, src_cap, CAP_RIGHT_MEMORY_UNMAP, &delegated_cap);
    assert(ret != 0); // Delegation rejected

    // Verify destination table has NO live capability
    g_current_cpu_id = 2;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(dst->entries); i++) {
        assert(dst->entries[i].in_use == 0 || dst->entries[i].state != CAP_STATE_LIVE);
    }

    printf("  -> Transactional Rollback Guarantee PASSED!\n");
}

// 5. Revocation Epoch Mismatch Protection
void test_level5_epoch_mismatch(void) {
    printf("Running Test Level 5.5: Revocation Epoch Mismatch Protection...\n");
    setup_host_test_environment();

    capability_table_t *table = &g_mock_cap_tables[0];
    uint32_t cap_id = 0;
    cap_table_grant(table, CAP_TYPE_MEMORY, 0xB000, CAP_RIGHT_MEMORY_MAP, &cap_id);

    uint32_t slot_idx = bh_cap_index(cap_id) - 1;
    // Set parent locator to epoch 5
    table->entries[slot_idx].parent = (bh_cap_locator_t){
        .cspace_id = table->cspace_id,
        .owner_core = 1,
        .slot = 10,
        .generation = 1,
        .revocation_epoch = 5,
    };

    // Prepare a revoke request from CPU 1 targeting slot 10, generation 1, with OLD epoch 3
    uint8_t tx_slot = 0;
    g_current_cpu_id = 1;
    bh_cap_tx_entry_t *tx = cap_tx_alloc(1, BH_CAP_TX_OP_REVOKE, &tx_slot);
    assert(tx != NULL);
    tx->target = (bh_cap_locator_t){
        .cspace_id = 0,
        .owner_core = 1,
        .slot = 10,
        .generation = 1,
        .revocation_epoch = 3, // Older epoch!
    };
    tx->revocation_epoch = 3;
    atomic_set(&tx->state, BH_CAP_TX_PUBLISHED);

    // Deliver revoke request to CPU 0
    g_current_cpu_id = 0;
    uint64_t req_payload = cap_tx_pack_req(1, tx_slot, tx->generation, (uint8_t)BH_CAP_TX_OP_REVOKE);
    cap_handle_tx_req(req_payload, 1);

    // Capability on CPU 0 must NOT be revoked because request epoch (3) < capability parent epoch (5)
    assert(table->entries[slot_idx].in_use == 1);
    assert(table->entries[slot_idx].state == CAP_STATE_LIVE);

    // Now send revoke request with matching/newer epoch 5
    tx->revocation_epoch = 5;
    cap_handle_tx_req(req_payload, 1);

    // Now it MUST be revoked
    assert(table->entries[slot_idx].in_use == 0 || table->entries[slot_idx].state == CAP_STATE_FREE);

    g_current_cpu_id = 1;
    cap_tx_release(1, tx_slot);
    printf("  -> Revocation Epoch Mismatch Protection PASSED!\n");
}

int main(void) {
    printf("=========================================\n");
    printf("Running Comprehensive Capability Security Host Tests\n");
    printf("=========================================\n");

    test_level1_validation();
    test_level2_service_authorization();
    test_level3_distributed_transactions();
    test_level4_multicore_integration();
    test_level5_duplicate_ack();
    test_level5_stale_ack_rejection();
    test_level5_lost_ack_timeout();
    test_level5_rollback_guarantee();
    test_level5_epoch_mismatch();

    printf("\n=========================================\n");
    printf("All CAPTX-P0-001 Distributed Transaction Security Tests PASSED!\n");
    printf("=========================================\n");
    return 0;
}
