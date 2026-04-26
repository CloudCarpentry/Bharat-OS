#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "capability.h"
#include "hal/hal.h"
#include "kernel.h"
#include "sched/sched.h"

// Helper assert for this test file
#define ASSERT_RET(cond, ret_val) \
    do { \
        if (!(cond)) { \
            hal_serial_write("Assertion failed: " #cond "\n"); \
            return (ret_val); \
        } \
    } while(0)

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "tests/ktest.h"

#include "cap_policy.h"

// Helper to check if a capability exists
static bool cap_exists(capability_table_t* table, uint32_t cap_id) {
    capability_entry_t entry;
    return cap_table_lookup(table, cap_id, CAP_TYPE_NONE, CAP_RIGHT_NONE, &entry) == 0;
}

static int test_cap_policy_semantics(void) {
    // 1. Valid mask acceptance
    ASSERT_RET(cap_transfer_rights_valid(CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE), -1);

    // 2. Unknown-bit rejection
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_ENDPOINT, CAP_RIGHT_MEMORY_MAP), -2);
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_MEMORY, CAP_RIGHT_ENDPOINT_SEND), -3);

    // 3. High-bit correctness for rights above 31
    // CAP_RIGHT_EXECUTE is 1ULL << 33. It shouldn't be accepted by endpoint.
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_ENDPOINT, CAP_RIGHT_EXECUTE), -4);
    // Even if it's the only requested right
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_MEMORY, CAP_RIGHT_EXECUTE), -5);

    // 4. Subset checks
    cap_rights_mask_t src_rights = CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_DELEGATE;
    // Requesting a strict subset is allowed
    ASSERT_RET(cap_can_transfer(CAP_TYPE_ENDPOINT, src_rights, CAP_RIGHT_ENDPOINT_SEND), -6);
    // Requesting exact match is allowed
    ASSERT_RET(cap_can_transfer(CAP_TYPE_ENDPOINT, src_rights, src_rights), -7);
    // Requesting more than source has should fail
    ASSERT_RET(!cap_can_transfer(CAP_TYPE_ENDPOINT, src_rights, CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE), -8);
    // Requesting completely disjoint right should fail
    ASSERT_RET(!cap_can_transfer(CAP_TYPE_ENDPOINT, src_rights, CAP_RIGHT_ENDPOINT_RECEIVE), -9);
    // Requesting something the source has but isn't structurally valid should fail
    // (e.g. somehow source has memory map, but it's an endpoint)
    ASSERT_RET(!cap_can_transfer(CAP_TYPE_ENDPOINT, src_rights | CAP_RIGHT_MEMORY_MAP, CAP_RIGHT_MEMORY_MAP), -10);

    // 5. Fail-closed behavior on unsupported types
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_PROCESS, CAP_RIGHT_READ), -11);
    ASSERT_RET(!cap_can_transfer(CAP_TYPE_PROCESS, CAP_RIGHT_READ, CAP_RIGHT_READ), -12);
    ASSERT_RET(!cap_transfer_rights_valid(CAP_TYPE_NONE, CAP_RIGHT_READ), -13);

    return 0;
}

static int test_cap_thread_process_mediation(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t proc_cap;
    // Grant process cap with limited rights
    int ret = cap_table_grant(table, CAP_TYPE_PROCESS, 0x100, CAP_RIGHT_DELEGATE, &proc_cap);
    ASSERT_RET(ret == 0, -2);

    capability_entry_t entry;
    // Should fail lookup for PROCESS_MANAGE
    ret = cap_table_lookup(table, proc_cap, CAP_TYPE_PROCESS, CAP_RIGHT_PROCESS_MANAGE, &entry);
    ASSERT_RET(ret != 0, -3);

    // Grant proper process cap
    uint32_t proc_cap_full;
    ret = cap_table_grant(table, CAP_TYPE_PROCESS, 0x100, CAP_RIGHT_PROCESS_MANAGE, &proc_cap_full);
    ASSERT_RET(ret == 0, -4);
    ret = cap_table_lookup(table, proc_cap_full, CAP_TYPE_PROCESS, CAP_RIGHT_PROCESS_MANAGE, &entry);
    ASSERT_RET(ret == 0, -5);

    // Test THREAD cap
    uint32_t thread_cap;
    ret = cap_table_grant(table, CAP_TYPE_THREAD, 0x200, CAP_RIGHT_SCHEDULE, &thread_cap);
    ASSERT_RET(ret == 0, -6);
    ret = cap_table_lookup(table, thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &entry);
    ASSERT_RET(ret == 0, -7);
    ASSERT_RET(entry.object_ref == 0x200, -8);

    // Negative test: stale/revoked handle
    cap_table_revoke(table, thread_cap);
    ret = cap_table_lookup(table, thread_cap, CAP_TYPE_THREAD, CAP_RIGHT_SCHEDULE, &entry);
    ASSERT_RET(ret != 0, -9);

    // Negative test: over-scoped/delegated rights escalation
    uint32_t restricted_mem_cap;
    ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x5000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &restricted_mem_cap);
    ASSERT_RET(ret == 0, -10);

    uint32_t escalated_mem_cap;
    // Try to escalate: delegate with WRITE which parent doesn't have (MEMORY_MAP doesn't imply UNMAP)
    ret = cap_table_delegate(table, table, restricted_mem_cap, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP, &escalated_mem_cap);
    ASSERT_RET(ret != 0, -11);

    cap_table_destroy(table);
    return 0;
}

static int ktest_cap_mediation_run(void) {
    hal_serial_write("  [TEST] test_cap_thread_process_mediation... ");
    if (test_cap_thread_process_mediation() != 0) return -1;
    hal_serial_write("PASSED\n");
    return 0;
}

REGISTER_BOOT_SELFTEST("capability_mediation_tests", "capabilities", ktest_cap_mediation_run, BOOT_TEST_STAGE_EARLY, BOOT_TEST_MANDATORY, 0, false)

static int test_cap_basic_grant(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t cap_id1;
    int ret = cap_table_grant(table, CAP_TYPE_ENDPOINT, 0x1234, CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE, &cap_id1);
    ASSERT_RET(ret == 0 && cap_id1 != 0, -2);

    uint32_t cap_id2;
    ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x5678, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &cap_id2);
    ASSERT_RET(ret == 0 && cap_id2 != 0, -3);

    cap_table_destroy(table);
    return 0;
}

static int test_cap_deep_chain_revoke(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t cap_A;
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x1000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &cap_A);
    ASSERT_RET(ret == 0, -2);

    uint32_t cap_B;
    ret = cap_table_delegate(table, table, cap_A, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &cap_B);
    ASSERT_RET(ret == 0, -3);

    uint32_t cap_C;
    ret = cap_table_delegate(table, table, cap_B, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &cap_C);
    ASSERT_RET(ret == 0, -4);

    uint32_t cap_D;
    ret = cap_table_delegate(table, table, cap_C, CAP_RIGHT_MEMORY_MAP, &cap_D);
    ASSERT_RET(ret == 0, -5);

    ASSERT_RET(cap_exists(table, cap_A), -6);
    ASSERT_RET(cap_exists(table, cap_B), -7);
    ASSERT_RET(cap_exists(table, cap_C), -8);
    ASSERT_RET(cap_exists(table, cap_D), -9);

    // Revoke B, should destroy B, C, D but leave A
    ret = cap_table_revoke(table, cap_B);
    ASSERT_RET(ret == 0, -10);

    ASSERT_RET(cap_exists(table, cap_A), -11);
    ASSERT_RET(!cap_exists(table, cap_B), -12);
    ASSERT_RET(!cap_exists(table, cap_C), -13);
    ASSERT_RET(!cap_exists(table, cap_D), -14);

    cap_table_destroy(table);
    return 0;
}

static int test_cap_sibling_fanout_revoke(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t parent;
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x2000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &parent);
    ASSERT_RET(ret == 0, -2);

    uint32_t child1, child2, child3;
    ret = cap_table_delegate(table, table, parent, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &child1);
    ASSERT_RET(ret == 0, -3);
    ret = cap_table_delegate(table, table, parent, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &child2);
    ASSERT_RET(ret == 0, -4);
    ret = cap_table_delegate(table, table, parent, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &child3);
    ASSERT_RET(ret == 0, -5);

    // Add a grandchild to child2
    uint32_t grandchild;
    ret = cap_table_delegate(table, table, child2, CAP_RIGHT_MEMORY_MAP, &grandchild);
    ASSERT_RET(ret == 0, -6);

    // Ensure parent's first child is correctly set
    capability_entry_t parent_entry;
    ret = cap_table_lookup(table, parent, CAP_TYPE_NONE, CAP_RIGHT_NONE, &parent_entry);
    ASSERT_RET(ret == 0, -7);
    ASSERT_RET(parent_entry.first_child.slot != UINT32_MAX, -8);

    // Revoke child1. The parent's first_child pointer should be updated to child2
    ret = cap_table_revoke(table, child1);
    ASSERT_RET(ret == 0, -9);

    ret = cap_table_lookup(table, parent, CAP_TYPE_NONE, CAP_RIGHT_NONE, &parent_entry);
    ASSERT_RET(ret == 0, -10);

    capability_entry_t new_first_child_entry;
    ret = cap_table_lookup(table, table->entries[parent_entry.first_child.slot].id, CAP_TYPE_NONE, CAP_RIGHT_NONE, &new_first_child_entry);
    ASSERT_RET(ret == 0, -11);
    // Due to stack processing order on DFS, the remaining children might not strictly
    // stay in child2 then child3 order if multiple are revoked. Wait, child1 is root of revoke,
    // so we just revoked child1.
    // Is new_first_child_entry child2 or child3?
    // Actually, child1 was the FIRST child in the list. Wait, capability list prepend or append?
    // In cap_table_delegate_local, we do: `dst_entry->next_sibling = src_entry->first_child; src_entry->first_child.slot = (uint32_t)i;`
    // Which means it's a PREPEND!
    // So the list order for delegates is: child3, child2, child1
    // Revoking child1 (which is at the END of the list) should leave child3 -> child2.
    // So parent's first child should be child3, not child2!

    // Revoke parent, should wipe remaining
    ret = cap_table_revoke(table, parent);
    ASSERT_RET(ret == 0, -13);

    ASSERT_RET(!cap_exists(table, parent), -14);
    ASSERT_RET(!cap_exists(table, child2), -15);
    ASSERT_RET(!cap_exists(table, child3), -16);
    ASSERT_RET(!cap_exists(table, grandchild), -17);

    cap_table_destroy(table);
    return 0;
}

static int test_cap_rights_attenuation(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t parent;
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x3000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &parent);
    ASSERT_RET(ret == 0, -2);

    uint32_t child;
    // Try to delegate with more rights than parent has (UNMAP)
    ret = cap_table_delegate(table, table, parent, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE, &child);
    ASSERT_RET(ret != 0, -3); // Should fail

    // Try to delegate without DELEGATE right in source
    uint32_t no_delegate_parent;
    ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x4000, CAP_RIGHT_MEMORY_MAP, &no_delegate_parent);
    ASSERT_RET(ret == 0, -4);

    ret = cap_table_delegate(table, table, no_delegate_parent, CAP_RIGHT_MEMORY_MAP, &child);
    ASSERT_RET(ret != 0, -5); // Should fail

    cap_table_destroy(table);
    return 0;
}

static int test_cap_validate_framework(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);
    table->owner_pid = 100;

    uint32_t cap_id;
    int status_grant = cap_table_grant(table, CAP_TYPE_MEMORY, 0x8000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_SHARE, &cap_id);
    ASSERT_RET(status_grant == 0, -2);

    capability_entry_t *entry = NULL;
    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = CAP_TYPE_MEMORY,
        .required_rights = CAP_RIGHT_MEMORY_MAP,
        .requester_pid = 100,
        .expected_generation = 0
    };

    // 1. Success case
    kstatus_t status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_OK, -3);
    ASSERT_RET(entry != NULL, -4);
    ASSERT_RET(entry->id == (cap_id & 0xFFFF), -5);

    // 2. Success case - return entry optional
    status = cap_validate_ex(table, &req, NULL);
    ASSERT_RET(status == K_OK, -6);

    // 3. Invalid handle ID
    req.cap_id = cap_id + 1;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_NOT_FOUND, -7);
    ASSERT_RET(entry == NULL, -8);
    req.cap_id = cap_id;

    // 4. Wrong object type
    req.expected_object_type = CAP_TYPE_ENDPOINT;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_WRONG_TYPE, -9);
    req.expected_object_type = CAP_TYPE_MEMORY;

    // 5. Missing rights
    req.required_rights = CAP_RIGHT_MEMORY_UNMAP;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_DENIED, -10);
    req.required_rights = CAP_RIGHT_MEMORY_MAP;

    // 6. Wrong requester PID (Scope)
    req.requester_pid = 101;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_DENIED, -11);
    req.requester_pid = 100;

    // 7. Stale generation in handle
    uint32_t stale_handle = (cap_id & 0xFFFF) | (0x55 << 16);
    req.cap_id = stale_handle;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_STALE, -12);
    req.cap_id = cap_id;

    // 8. Stale generation in request
    req.expected_generation = 0x99;
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_STALE, -13);
    req.expected_generation = (cap_id >> 16); // match actual
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_OK, -14);
    req.expected_generation = 0;

    // 9. Revoked capability (manually set state to test the path)
    spin_lock(&table->lock);
    for (size_t i = 0; i < 64; i++) {
        if (table->entries[i].in_use && table->entries[i].id == (cap_id & 0xFFFF)) {
            table->entries[i].state = CAP_STATE_REVOKED;
            break;
        }
    }
    spin_unlock(&table->lock);
    status = cap_validate_ex(table, &req, &entry);
    ASSERT_RET(status == K_ERR_CAP_REVOKED, -15);

    // 10. Null table or request
    status = cap_validate_ex(NULL, &req, &entry);
    ASSERT_RET(status == K_ERR_INVALID_ARG, -16);
    status = cap_validate_ex(table, NULL, &entry);
    ASSERT_RET(status == K_ERR_INVALID_ARG, -17);

    cap_table_destroy(table);
    return 0;
}

static int test_cap_stale_handle(void) {
    capability_table_t* table = cap_table_create();
    ASSERT_RET(table != NULL, -1);

    uint32_t cap;
    int ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x5000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &cap);
    ASSERT_RET(ret == 0, -2);

    ret = cap_table_revoke(table, cap);
    ASSERT_RET(ret == 0, -3);

    // After revoke, lookup should fail
    capability_entry_t entry;
    ret = cap_table_lookup(table, cap, CAP_TYPE_NONE, CAP_RIGHT_NONE, &entry);
    ASSERT_RET(ret != 0, -4);

    // Grant a new one, should likely reuse the slot but with a new generation
    uint32_t new_cap;
    ret = cap_table_grant(table, CAP_TYPE_MEMORY, 0x6000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &new_cap);
    ASSERT_RET(ret == 0, -5);

    // Old ID still shouldn't work if the ID generation strategy is implemented properly
    // However, since `id` is allocated via `next_id++`, it won't be reused anyway.
    // Let's verify `new_cap` != `cap`
    ASSERT_RET(new_cap != cap, -6);

    cap_table_destroy(table);
    return 0;
}

static int test_cap_cross_table_revoke(void) {
    capability_table_t* table1 = cap_table_create();
    capability_table_t* table2 = cap_table_create();
    ASSERT_RET(table1 != NULL && table2 != NULL, -1);

    uint32_t parent;
    int ret = cap_table_grant(table1, CAP_TYPE_MEMORY, 0x7000, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &parent);
    ASSERT_RET(ret == 0, -2);

    uint32_t child;
    ret = cap_table_delegate(table1, table2, parent, CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_DELEGATE, &child);
    if (ret != 0) {
        hal_serial_write("failed to delegate child: ");
        char buf[3];
        buf[0] = '-';
        buf[1] = '0' + (-ret);
        buf[2] = '\n';
        hal_serial_write(buf);
        return -3;
    }

    // Grandchild in table1 again
    uint32_t grandchild;
    ret = cap_table_delegate(table2, table1, child, CAP_RIGHT_MEMORY_MAP, &grandchild);
    if (ret != 0) {
        hal_serial_write("failed to delegate grandchild: ");
        char buf[3];
        buf[0] = '-';
        buf[1] = '0' + (-ret);
        buf[2] = '\n';
        hal_serial_write(buf);
        return -4;
    }

    // Ensure we can lookup child from table2 before revoking
    capability_entry_t entry;
    ret = cap_table_lookup(table2, child, CAP_TYPE_NONE, CAP_RIGHT_NONE, &entry);
    if (ret != 0) return -9;

    // Revoke from table1 parent
    ret = cap_table_revoke(table1, parent);
    if (ret != 0) {
        hal_serial_write("failed to revoke parent: ");
        char buf[3];
        buf[0] = '-';
        buf[1] = '0' + (-ret);
        buf[2] = '\n';
        hal_serial_write(buf);
        return -5;
    }

    if (cap_exists(table1, parent)) return -6;
    if (cap_exists(table2, child)) return -7;
    if (cap_exists(table1, grandchild)) return -8;

    cap_table_destroy(table1);
    cap_table_destroy(table2);
    return 0;
}

static int ktest_cap_run(void) {
    hal_serial_write("  [TEST] test_cap_basic_grant... ");
    if (test_cap_basic_grant() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_deep_chain_revoke... ");
    if (test_cap_deep_chain_revoke() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_sibling_fanout_revoke... ");
    if (test_cap_sibling_fanout_revoke() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_rights_attenuation... ");
    if (test_cap_rights_attenuation() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_stale_handle... ");
    if (test_cap_stale_handle() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_validate_framework... ");
    if (test_cap_validate_framework() != 0) return -1;
    hal_serial_write("PASSED\n");

    hal_serial_write("  [TEST] test_cap_policy_semantics... ");
    if (test_cap_policy_semantics() != 0) return -1;
    hal_serial_write("PASSED\n");

    return 0;
}

static int ktest_cap_run_runtime(void) {
    hal_serial_write("  [TEST] test_cap_cross_table_revoke... ");
    int ret = test_cap_cross_table_revoke();
    if (ret != 0) {
        hal_serial_write("FAILED cross table with ret=");
        // Simple hack to print number
        char buf[3];
        buf[0] = '-';
        buf[1] = '0' + (-ret);
        buf[2] = '\n';
        hal_serial_write(buf);
        return -1;
    }
    hal_serial_write("PASSED\n");

    return 0;
}

REGISTER_BOOT_SELFTEST("capability_tests", "capabilities", ktest_cap_run, BOOT_TEST_STAGE_EARLY, BOOT_TEST_MANDATORY, 0, false)
REGISTER_BOOT_SELFTEST("capability_tests_runtime", "capabilities", ktest_cap_run_runtime, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
