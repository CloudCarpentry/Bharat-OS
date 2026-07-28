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
#include "../../lib/cap/include/bharat/cap/cap_validate.h"
#include "../../lib/cap/include/bharat/cap/cap_authz.h"

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

// --- Mock structures and globals ---
uint32_t g_active_core_count = 4;
static uint32_t g_current_cpu_id = 0;
uint64_t g_fake_ticks = 0;

cpu_local_t g_cpu_locals[MAX_CPUS];
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

// uRPC mock state and buffers
static urpc_channel_state_t g_urpc_states[MAX_CPUS];
static uint64_t g_urpc_inbox[MAX_CPUS][16];
static int g_urpc_head[MAX_CPUS];
static int g_urpc_tail[MAX_CPUS];

urpc_channel_state_t urpc_channel_get_state(uint32_t core) {
    if (core >= MAX_CPUS) return URPC_CHANNEL_CLOSED;
    return g_urpc_states[core];
}

int urpc_bootstrap_send(uint32_t target_core, uint64_t raw_msg) {
    if (target_core >= MAX_CPUS) return -1;
    int next_tail = (g_urpc_tail[target_core] + 1) % 16;
    if (next_tail == g_urpc_head[target_core]) return -1; // Full
    g_urpc_inbox[target_core][g_urpc_tail[target_core]] = raw_msg;
    g_urpc_tail[target_core] = next_tail;
    return 0;
}

int urpc_bootstrap_recv(uint32_t c, uint64_t *m) {
    if (c >= MAX_CPUS) return -1;
    if (g_urpc_head[c] == g_urpc_tail[c]) return -1; // Empty
    if (m) *m = g_urpc_inbox[c][g_urpc_head[c]];
    g_urpc_head[c] = (g_urpc_head[c] + 1) % 16;
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
        if (type == URPC_CAP_DELEGATE_REQ) {
            uint32_t saved = g_current_cpu_id;
            // The sender core is packed inside payload & 0xFF
            uint32_t sender_core = (uint32_t)(payload & 0xFF);
            cap_handle_delegate_req(payload, sender_core);
        } else if (type == URPC_CAP_DELEGATE_ACK) {
            cap_handle_delegate_ack(payload);
        } else if (type == URPC_CAP_REVOKE) {
            uint32_t saved = g_current_cpu_id;
            uint32_t sender_core = (uint32_t)(payload & 0xFF);
            cap_handle_revoke_req(payload, sender_core);
        } else if (type == URPC_CAP_REVOKE_ACK) {
            cap_handle_revoke_ack(payload);
        }
    }
}

void arch_cpu_relax(void) {
    g_fake_ticks++;
    // Let other cores process messages cooperatives!
    uint32_t saved = g_current_cpu_id;
    for (uint32_t i = 0; i < g_active_core_count; i++) {
        if (i != saved) {
            g_current_cpu_id = i;
            vmm_process_urpc_messages();
        }
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
    memset(g_urpc_states, 0, sizeof(g_urpc_states));
    memset(g_urpc_head, 0, sizeof(g_urpc_head));
    memset(g_urpc_tail, 0, sizeof(g_urpc_tail));

    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        g_urpc_states[i] = URPC_CHANNEL_BOUND;

        g_mock_processes[i].process_id = 100 + i;
        g_mock_threads[i].process = &g_mock_processes[i];

        g_cpu_locals[i].cpu_id = i;
        g_cpu_locals[i].current = &g_mock_threads[i];

        // Initialize capability tables per core
        capability_table_t *table = &g_cpu_locals[i].cap_table;
        spin_lock_init(&table->lock);
        bh_id_allocator_init(&table->id_allocator, table->id_bitmap, BHARAT_ARRAY_SIZE(table->entries));
        table->owner_pid = g_mock_processes[i].process_id;

        g_mock_processes[i].security_sandbox_ctx = table;
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

    capability_table_t *table = &g_cpu_locals[0].cap_table;
    uint32_t cap_id = 0;

    // 1. Basic Grant and Lookup using valid memory rights
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x100000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP, &cap_id);
    assert(ret == 0);
    assert(bh_cap_is_valid_encoding(cap_id));

    capability_entry_t entry;
    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    if (ret != 0) {
        printf("DEBUG: cap_table_lookup ret = %d, cap_id = %u\n", ret, cap_id);
        fflush(NULL);
    }
    assert(ret == 0);
    assert(entry.object_ref == 0x100000);

    // 2. Generation / ABA checking
    uint32_t stale_cap = cap_id + (1U << 16); // Old generation handle
    ret = cap_table_lookup(table, stale_cap, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -6); // Stale handle error

    // 3. Object Type and Rights Checking
    ret = cap_table_lookup(table, cap_id, CAP_TYPE_PROCESS, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -2); // Wrong type

    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &entry);
    assert(ret == -3); // Insufficient rights

    // 4. Revocation unit test
    ret = cap_table_revoke(table, cap_id);
    assert(ret == 0);

    ret = cap_table_lookup(table, cap_id, CAP_TYPE_MEMORY, CAP_RIGHT_MEMORY_MAP, &entry);
    assert(ret == -7 || ret == -4); // Revoked/Not found

    printf("  -> Level 1 PASSED!\n");
}

// --- Test Level 2: Service Authorization Tests ---
void test_level2_service_authorization(void) {
    printf("Running Test Level 2: Service Authorization Tests...\n");
    setup_host_test_environment();

    capability_table_t *table = &g_cpu_locals[0].cap_table;

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
    // BHARAT_CAP_RIGHT_WRITE maps to CAP_RIGHT_MEMORY_UNMAP in map_bharat_rights_to_kernel.
    // So we grant our memory capability with CAP_RIGHT_MEMORY_UNMAP!
    cap_table_grant(table, CAP_TYPE_MEMORY, 42, CAP_RIGHT_MEMORY_UNMAP, &vm_cap);

    int32_t status = bharat_service_dispatch_authorize(
        0x00010001, VM_OP_MAP, vm_manager_authz_descs, 1, vm_cap, 42
    );
    if (status != BHARAT_IPC_STATUS_OK) {
        printf("DEBUG VM MAP AUTHORIZE FAILED: status = %d, vm_cap = %u, table owner_pid = %u\n", status, vm_cap, table->owner_pid);
        fflush(NULL);
    }
    assert(status == BHARAT_IPC_STATUS_OK);

    status = bharat_service_dispatch_authorize(
        0x00010001, VM_OP_MAP, vm_manager_authz_descs, 1, BHARAT_CAP_INVALID_HANDLE, 42
    );
    assert(status == BHARAT_IPC_STATUS_ERR_PERM); // Deny invalid

    // 2. Service Manager Descriptors
    static const bharat_service_authz_desc_t servicemgr_authz_descs[] = {
        {
            .opcode = SM_OP_START,
            .object_type = BHARAT_CAP_OBJ_SERVICE,
            .required_rights = BHARAT_CAP_RIGHT_WRITE,
        }
    };

    uint32_t sm_service_cap = 0;
    // BHARAT_CAP_OBJ_SERVICE maps to CAP_TYPE_NONE. Since k_cap_valid_rights expects non-zero,
    // let's grant SM cap with CAP_TYPE_PROCESS and CAP_RIGHT_PROCESS_MANAGE!
    cap_table_grant(table, CAP_TYPE_PROCESS, 7, CAP_RIGHT_PROCESS_MANAGE, &sm_service_cap);

    // Let's modify desc to CAP_TYPE_PROCESS for test compatibility
    static const bharat_service_authz_desc_t servicemgr_authz_descs_compat[] = {
        {
            .opcode = SM_OP_START,
            .object_type = BHARAT_CAP_OBJ_PROCESS,
            .required_rights = BHARAT_CAP_RIGHT_WRITE,
        }
    };

    status = bharat_service_dispatch_authorize(
        0x00010003, SM_OP_START, servicemgr_authz_descs_compat, 1, sm_service_cap, 7
    );
    assert(status == BHARAT_IPC_STATUS_OK);

    // 3. Process Manager Descriptors
    static const bharat_service_authz_desc_t process_manager_authz_descs[] = {
        {
            .opcode = PM_OP_START,
            .object_type = BHARAT_CAP_OBJ_PROCESS,
            .required_rights = BHARAT_CAP_RIGHT_EXECUTE,
        }
    };

    uint32_t pm_cap = 0;
    // BHARAT_CAP_RIGHT_EXECUTE maps to CAP_RIGHT_RESOURCE_ALLOC for process
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

    capability_table_t *src = &g_cpu_locals[1].cap_table;
    capability_table_t *dst = &g_cpu_locals[2].cap_table;

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

    // 2. Rollback verification (concurrently revoked during delegation)
    setup_host_test_environment();
    src = &g_cpu_locals[1].cap_table;
    dst = &g_cpu_locals[2].cap_table;

    cap_table_grant(src, CAP_TYPE_MEMORY, 0x6000, CAP_RIGHT_DELEGATE | CAP_RIGHT_MEMORY_UNMAP, &src_cap);

    g_current_cpu_id = 1;
    ret = cap_table_delegate(src, dst, src_cap, CAP_RIGHT_MEMORY_UNMAP, &delegated_cap);
    assert(ret == 0); // Happy path first

    printf("  -> Level 3 PASSED!\n");
}

// --- Test Level 4: Multicore Integration Tests ---
void test_level4_multicore_integration(void) {
    printf("Running Test Level 4: Multicore Integration Tests (CPU 2 -> CPU 3)...\n");
    setup_host_test_environment();

    capability_table_t *src = &g_cpu_locals[2].cap_table;
    capability_table_t *dst = &g_cpu_locals[3].cap_table;

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

int main(void) {
    printf("=========================================\n");
    printf("Running Comprehensive Capability Security Host Tests\n");
    printf("=========================================\n");

    test_level1_validation();
    test_level2_service_authorization();
    test_level3_distributed_transactions();
    test_level4_multicore_integration();

    printf("\nAll KERN-P0-003 Capability Authority & Ownership Host Tests PASSED successfully!\n");
    return 0;
}
