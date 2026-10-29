#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "trap/syscall_context.h"
#include "syscall/usercopy.h"
#include "capability/cap_lookup.h"
#include "ipc_endpoint.h"
#include "sched/sched.h"
#include <bharat/uapi/syscall_args.h>
#include <bharat/uapi/ipc/envelope.h>

static unsigned calls, revoked;
static const void *user_payload;
static void *fail_copyout;
static bool fail_copyin, deny_cap, fail_range;
static capability_table_t table;

bh_status_t bh_copy_from_user(void *dst, const void *src, size_t size) {
    if (fail_copyin && src == user_payload) return BH_ERR_FAULT;
    memcpy(dst, src, size);
    return BH_OK;
}
bh_status_t bh_copy_to_user(void *dst, const void *src, size_t size) {
    if (dst == fail_copyout) return BH_ERR_FAULT;
    memcpy(dst, src, size);
    return BH_OK;
}
bh_status_t bh_user_range_validate(const void *ptr, size_t size, bh_user_access_t access) {
    (void)ptr; (void)size;
    assert(access == BH_USER_ACCESS_WRITE || access == BH_USER_ACCESS_READ);
    return fail_range ? BH_ERR_FAULT : BH_OK;
}
kstatus_t cap_lookup_endpoint(const capability_table_t *t, uint32_t cap,
    cap_rights_mask_t rights, bh_endpoint_object_t *out) {
    assert(t == &table && cap == 1);
    assert(rights == CAP_RIGHT_ENDPOINT_SEND || rights == CAP_RIGHT_ENDPOINT_RECEIVE);
    (void)out;
    return deny_cap ? K_ERR_CAP_DENIED : K_OK;
}
int cap_table_revoke(capability_table_t *t, uint32_t cap) {
    assert(t == &table && cap == 77);
    ++revoked;
    return 0;
}
int ipc_endpoint_send(capability_table_t *t, uint32_t cap, const void *payload,
    uint32_t len, uint64_t timeout, uint32_t transferred, uint64_t rights) {
    assert(t == &table && cap == 1 && payload != user_payload && len == 4);
    assert(memcmp(payload, user_payload, len) == 0);
    (void)timeout; (void)transferred; (void)rights;
    ++calls;
    return IPC_OK;
}
int ipc_endpoint_send_ex(capability_table_t *t, uint32_t cap, const void *payload,
    uint32_t len, uint64_t timeout, uint32_t transferred, uint64_t rights,
    const ipc_provenance_t *provenance) {
    assert(t == &table && cap == 1 && payload != user_payload && len == 4);
    assert(memcmp(payload, user_payload, len) == 0);
    assert(provenance != NULL);
    assert(provenance->has_provenance == 1U);
    assert(provenance->sender_pid == 1234);
    assert(provenance->sender_process_object_id == 5678);
    assert(provenance->sender_cspace_id == 42);
    assert(provenance->sender_owner_core == 2);
    (void)timeout; (void)transferred; (void)rights;
    ++calls;
    return IPC_OK;
}
int ipc_endpoint_receive(capability_table_t *t, uint32_t cap, void *payload,
    uint32_t capacity, uint32_t *len, uint64_t timeout, uint32_t *received_cap) {
    assert(t == &table && cap == 1 && payload != user_payload && capacity >= 4);
    (void)timeout;
    memcpy(payload, "test", 4);
    *len = 4;
    *received_cap = 77;
    ++calls;
    return IPC_OK;
}
int ipc_endpoint_receive_envelope(capability_table_t *t, uint32_t cap, void *payload,
    uint32_t capacity, uint32_t *len, uint64_t timeout, bh_ipc_receive_envelope_v1_t *out_envelope) {
    assert(t == &table && cap == 1 && payload != user_payload && capacity >= 4);
    (void)timeout;
    memcpy(payload, "test", 4);
    *len = 4;
    assert(out_envelope != NULL);
    memset(out_envelope, 0, sizeof(*out_envelope));
    out_envelope->version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1;
    out_envelope->struct_size = (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t);
    out_envelope->valid_fields = BH_IPC_ENVELOPE_F_SENDER_PID |
                                 BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT |
                                 BH_IPC_ENVELOPE_F_SENDER_CSPACE |
                                 BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE |
                                 BH_IPC_ENVELOPE_F_REPLY_SEND_CAP;
    out_envelope->sender_pid = 1234;
    out_envelope->sender_process_object_id = 5678;
    out_envelope->sender_cspace_id = 42;
    out_envelope->sender_owner_core = 2;
    out_envelope->delivery_generation = 1;
    out_envelope->reply_send_cap = 77;
    ++calls;
    return IPC_OK;
}

extern bh_operation_result_t bh_sys_endpoint_send(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_endpoint_receive(bh_syscall_ctx_t *ctx);

int main(void) {
    table.cspace_id = 42;
    table.owner_core = 2;
    bh_process_t process = {
        .process_id = 1234,
        .object_id = 5678,
        .security_sandbox_ctx = &table,
    };
    bh_syscall_ctx_t ctx = {.process = &process};
    char payload[4] = "test";
    user_payload = payload;

    /* 1. Test endpoint send with provenance capture */
    bharat_sys_endpoint_send_args_t send = {
        .send_cap = 1,
        .payload_len = 4,
        .payload_ptr = (uintptr_t)payload
    };
    ctx.regs.arg[0] = (uintptr_t)&send;
    assert(bh_sys_endpoint_send(&ctx).value == K_OK && calls == 1);
    fail_copyin = true;
    assert(bh_sys_endpoint_send(&ctx).value == K_ERR_FAULT && calls == 1);
    fail_copyin = false;
    send.payload_len = BHARAT_IPC_ENDPOINT_PAYLOAD_MAX + 1;
    assert(bh_sys_endpoint_send(&ctx).value == K_ERR_INVALID_ARG && calls == 1);
    send.payload_len = 4;
    deny_cap = true;
    assert(bh_sys_endpoint_send(&ctx).value == K_ERR_CAP_DENIED && calls == 1);
    deny_cap = false;

    /* 2. Test legacy receive (selector == 0) */
    ctx.regs.arg[1] = BH_IPC_RECEIVE_SELECTOR_LEGACY;
    uint32_t length = 0, cap = 0;
    bharat_sys_endpoint_receive_args_t receive = {
        .recv_cap = 1,
        .out_payload_ptr = (uintptr_t)payload,
        .out_payload_capacity = sizeof(payload),
        .out_len_ptr = (uintptr_t)&length,
        .out_received_cap_ptr = (uintptr_t)&cap
    };
    ctx.regs.arg[0] = (uintptr_t)&receive;
    memset(payload, 0, sizeof(payload));
    assert(bh_sys_endpoint_receive(&ctx).value == K_OK);
    assert(length == 4 && cap == 77 && memcmp(payload, "test", 4) == 0);
    void *destinations[] = {payload, &length, &cap};
    for (unsigned i = 0; i < 3; ++i) {
        fail_copyout = destinations[i];
        assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_FAULT);
        assert(revoked == i + 1);
    }
    fail_copyout = NULL;
    fail_range = true;
    unsigned previous = calls;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_FAULT && calls == previous);
    fail_range = false;
    receive.out_received_cap_ptr = 0;
    assert(bh_sys_endpoint_receive(&ctx).value == K_OK && revoked == 4);

    /* 3. Test versioned authenticated envelope receive (selector == 1) */
    ctx.regs.arg[1] = BH_IPC_RECEIVE_SELECTOR_V1;
    bh_ipc_receive_envelope_v1_t envelope;
    memset(&envelope, 0xFF, sizeof(envelope));
    bharat_sys_endpoint_receive_v1_args_t v1_recv = {
        .version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1,
        .struct_size = (uint32_t)sizeof(bharat_sys_endpoint_receive_v1_args_t),
        .flags = 0,
        .recv_cap = 1,
        .out_payload_capacity = sizeof(payload),
        .out_envelope_capacity = (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t),
        .out_payload_ptr = (uintptr_t)payload,
        .out_len_ptr = (uintptr_t)&length,
        .out_envelope_ptr = (uintptr_t)&envelope,
        .timeout_ticks = 0,
        .reserved0 = 0,
    };
    ctx.regs.arg[0] = (uintptr_t)&v1_recv;
    memset(payload, 0, sizeof(payload));
    length = 0;

    /* 3a. Positive test */
    assert(bh_sys_endpoint_receive(&ctx).value == K_OK);
    assert(length == 4 && memcmp(payload, "test", 4) == 0);
    assert(envelope.version == BH_IPC_RECEIVE_ENVELOPE_VERSION_V1);
    assert(envelope.struct_size == (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t));
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PID) != 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT) != 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_SENDER_CSPACE) != 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE) != 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_REPLY_SEND_CAP) != 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_AUTHORITY_CAP) == 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_BOOT_SESSION) == 0);
    assert((envelope.valid_fields & BH_IPC_ENVELOPE_F_SERVICE_INCARNATION) == 0);
    assert(envelope.sender_pid == 1234);
    assert(envelope.sender_process_object_id == 5678);
    assert(envelope.sender_cspace_id == 42);
    assert(envelope.sender_owner_core == 2);
    assert(envelope.delivery_generation == 1);
    assert(envelope.reply_send_cap == 77);
    assert(envelope.authority_cap == 0);
    assert(envelope.boot_session_id == 0);
    assert(envelope.service_incarnation_id == 0);

    /* 3b. Negative tests: invalid selector */
    ctx.regs.arg[1] = 2;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    ctx.regs.arg[1] = 99;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    ctx.regs.arg[1] = BH_IPC_RECEIVE_SELECTOR_V1;

    /* 3c. Negative tests: version, size, flags, reserved0, capacity */
    v1_recv.version = 0;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.version = 2;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1;

    v1_recv.struct_size = sizeof(bharat_sys_endpoint_receive_v1_args_t) - 1;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.struct_size = sizeof(bharat_sys_endpoint_receive_v1_args_t) + 4;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.struct_size = sizeof(bharat_sys_endpoint_receive_v1_args_t);

    v1_recv.flags = 1;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.flags = 0;

    v1_recv.reserved0 = 1;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.reserved0 = 0;

    v1_recv.out_envelope_capacity = sizeof(bh_ipc_receive_envelope_v1_t) - 1;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.out_envelope_capacity = sizeof(bh_ipc_receive_envelope_v1_t);

    v1_recv.out_envelope_ptr = 0;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.out_envelope_ptr = (uintptr_t)&envelope;

    v1_recv.out_payload_capacity = 0;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_INVALID_ARG);
    v1_recv.out_payload_capacity = sizeof(payload);

    /* 3d. Range validation failure */
    fail_range = true;
    previous = calls;
    assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_FAULT && calls == previous);
    fail_range = false;

    /* 3e. Copyout fault rollback */
    unsigned initial_revoked = revoked;
    void *v1_destinations[] = {payload, &length, &envelope};
    for (unsigned i = 0; i < 3; ++i) {
        fail_copyout = v1_destinations[i];
        assert(bh_sys_endpoint_receive(&ctx).value == K_ERR_FAULT);
        assert(revoked == initial_revoked + i + 1);
    }
    fail_copyout = NULL;

    puts("PASS: endpoint fault-safe payload copies, bounds, envelope provenance, and rollback");
    return 0;
}
