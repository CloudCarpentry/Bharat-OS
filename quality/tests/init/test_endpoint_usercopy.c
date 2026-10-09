#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "trap/syscall_context.h"
#include "syscall/usercopy.h"
#include "capability/cap_lookup.h"
#include "ipc_endpoint.h"
#include "sched/sched.h"
#include <bharat/uapi/syscall_args.h>

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
extern bh_operation_result_t bh_sys_endpoint_send(bh_syscall_ctx_t *ctx);
extern bh_operation_result_t bh_sys_endpoint_receive(bh_syscall_ctx_t *ctx);

int main(void) {
    bh_process_t process = {.security_sandbox_ctx = &table};
    bh_syscall_ctx_t ctx = {.process = &process};
    char payload[4] = "test";
    user_payload = payload;
    bharat_sys_endpoint_send_args_t send = {.send_cap = 1, .payload_len = 4,
        .payload_ptr = (uintptr_t)payload};
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

    uint32_t length = 0, cap = 0;
    bharat_sys_endpoint_receive_args_t receive = {.recv_cap = 1,
        .out_payload_ptr = (uintptr_t)payload, .out_payload_capacity = sizeof(payload),
        .out_len_ptr = (uintptr_t)&length, .out_received_cap_ptr = (uintptr_t)&cap};
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
    puts("PASS: endpoint fault-safe payload copies, bounds, and transferred-cap cleanup");
}
