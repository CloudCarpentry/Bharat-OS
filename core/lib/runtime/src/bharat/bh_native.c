#include <bharat/bh_native.h>
#include <interface/uapi/syscall/syscall_nr.h>
#include <core/lib/syscall/common/syscall.h>

int bh_cap_revoke(bh_cap_t cap) {
    return (int)bh_syscall(SYSCALL_CAP_REVOKE, (long)cap, 0, 0, 0, 0, 0);
}

int bh_cap_transfer(bh_cap_t cap, bh_cap_t dest_ep) {
    return (int)bh_syscall(SYSCALL_CAP_TRANSFER, (long)cap, (long)dest_ep, 0, 0, 0, 0);
}

int bh_handle_close(bh_handle_t handle) {
    return (int)bh_syscall(SYSCALL_HANDLE_CLOSE, (long)handle, 0, 0, 0, 0, 0);
}

int bh_ipc_call(bh_endpoint_t ep, struct bh_ipc_msg* msg) {
    return (int)bh_syscall(SYSCALL_IPC_CALL, (long)ep, (long)msg, 0, 0, 0, 0);
}

int bh_alloc_ex(size_t size, uint32_t mem_class, uint32_t flags, void** out_addr) {
    uint64_t addr;
    long ret = bh_syscall(SYSCALL_MEM_ALLOC, (long)size, (long)mem_class, (long)flags, (long)&addr, 0, 0);
    if (ret == 0 && out_addr) {
        *out_addr = (void*)addr;
    }
    return (int)ret;
}

int bh_time_get(uint32_t clock_id, bh_time_t* out_time) {
    return (int)bh_syscall(SYSCALL_TIME_GET, (long)clock_id, (long)out_time, 0, 0, 0, 0);
}
