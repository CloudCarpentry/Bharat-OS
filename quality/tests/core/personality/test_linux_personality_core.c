#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "linux_compat.h"
#include "linux_errno.h"
#include "linux_futex.h"
#include "linux_personality.h"
#include "linux_syscall_numbers_x86_64.h"

// Mock kernel structures
static bh_process_t g_test_proc;
static bh_thread_t g_test_thread;
static uint64_t g_mock_time_ns = 1000000000ULL;

bh_process_t *sched_current_process(void) { return &g_test_proc; }
bh_thread_t *sched_current_thread(void) { return &g_test_thread; }
uint64_t bh_ktime_now(void) { return g_mock_time_ns++; }
void sched_sleep(uint64_t ms) { (void)ms; }
void sched_wakeup(bh_thread_t *th) { (void)th; }
void sched_block(void) {}
void bh_thread_yield(void) {}

long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) { (void)frame; (void)info; return 0; }
kstatus_t vm_map_region(address_space_t *as, const vm_map_request_t *req, uintptr_t *out) { (void)as; (void)req; if (out) *out = 0x7FFF0000; return K_OK; }
kstatus_t vm_protect_region(address_space_t *as, uintptr_t addr, size_t len, uint32_t prot) { (void)as; (void)addr; (void)len; (void)prot; return K_OK; }
kstatus_t vm_unmap_region(address_space_t *as, uintptr_t addr, size_t len) { (void)as; (void)addr; (void)len; return K_OK; }

static bh_thread_t g_spawned_thread;
bh_thread_t *thread_create_detached(bh_process_t *parent, void (*entry_point)(void)) {
    (void)entry_point;
    memset(&g_spawned_thread, 0, sizeof(g_spawned_thread));
    g_spawned_thread.thread_id = 42;
    g_spawned_thread.process = parent;
    return &g_spawned_thread;
}

kstatus_t sched_enqueue(bh_thread_t *thread, uint32_t core) {
    (void)thread; (void)core;
    return K_OK;
}

bh_status_t bh_copy_from_user(void *dst, const void *src, size_t n) {
    if (!dst || !src) return BH_ERR_FAULT;
    memcpy(dst, src, n);
    return BH_OK;
}

bh_status_t bh_copy_to_user(void *dst, const void *src, size_t n) {
    if (!dst || !src) return BH_ERR_FAULT;
    memcpy(dst, src, n);
    return BH_OK;
}

bh_status_t bh_user_range_validate(const void *base, size_t len, uint32_t access) {
    (void)base; (void)len; (void)access;
    return BH_OK;
}

kstatus_t bh_status_to_kstatus(bh_status_t st) { return (kstatus_t)st; }

bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_value(0);
}

bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx) {
    return bh_op_result_value((long)ctx->regs.arg[2]);
}

bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx) {
    (void)ctx;
    return bh_op_result_value(0);
}

void bh_translation_event_record(int event) { (void)event; }

struct linux_new_utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

typedef struct {
    uint64_t rlim_cur;
    uint64_t rlim_max;
} linux_rlimit64_t;

int main(void) {
    printf("Starting comprehensive Linux Personality Core verification...\n");

    subsys_instance_t env;
    memset(&env, 0, sizeof(env));
    env.type = SUBSYS_TYPE_LINUX;
    env.is_running = 1;

    assert(linux_subsys_init(&env) == 0);

    g_test_proc.process_id = 1234;
    g_test_thread.thread_id = 5678;
    g_test_thread.process = &g_test_proc;

    // 1. Verify getpid & gettid
    long pid = linux_syscall_handler(LINUX_X86_64_SYS_GETPID, 0, 0, 0, 0, 0, 0);
    printf("getpid -> %ld (expected 1234)\n", pid);
    assert(pid == 1234);

    long tid = linux_syscall_handler(LINUX_X86_64_SYS_GETTID, 0, 0, 0, 0, 0, 0);
    printf("gettid -> %ld (expected 5678)\n", tid);
    assert(tid == 5678);

    // 2. Verify openat on /dev/null
    const char *p_null = "/dev/null";
    long fd_null = linux_syscall_handler(LINUX_X86_64_SYS_OPENAT, 0, (long)p_null, 0, 0, 0, 0);
    printf("openat(/dev/null) -> fd %ld\n", fd_null);
    assert(fd_null >= 3);

    // 3. Verify read on /dev/null (should return 0 EOF)
    char buf[64];
    long read_ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, fd_null, (long)buf, sizeof(buf), 0, 0, 0);
    printf("read(/dev/null) -> %ld (expected 0)\n", read_ret);
    assert(read_ret == 0);

    // 4. Verify write on /dev/null (should return byte count)
    long write_ret = linux_syscall_handler(LINUX_X86_64_SYS_WRITE, fd_null, (long)"hello", 5, 0, 0, 0);
    printf("write(/dev/null) -> %ld (expected 5)\n", write_ret);
    assert(write_ret == 5);

    // 5. Verify close(/dev/null)
    long close_ret = linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, fd_null, 0, 0, 0, 0, 0);
    printf("close(/dev/null) -> %ld (expected 0)\n", close_ret);
    assert(close_ret == 0);

    // 6. Verify closing closed fd returns -EBADF (-9)
    long close_bad = linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, fd_null, 0, 0, 0, 0, 0);
    printf("close(closed_fd) -> %ld (expected -%d)\n", close_bad, LINUX_EBADF);
    assert(close_bad == -LINUX_EBADF);

    // 7. Verify openat on /dev/zero
    const char *p_zero = "/dev/zero";
    long fd_zero = linux_syscall_handler(LINUX_X86_64_SYS_OPENAT, 0, (long)p_zero, 0, 0, 0, 0);
    assert(fd_zero >= 3);
    memset(buf, 0xAA, sizeof(buf));
    read_ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, fd_zero, (long)buf, 16, 0, 0, 0);
    assert(read_ret == 16);
    for (int i = 0; i < 16; i++) {
        assert(buf[i] == 0);
    }
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, fd_zero, 0, 0, 0, 0, 0) == 0);

    // 8. Verify getrandom
    uint8_t rand_buf[32] = {0};
    long rand_ret = linux_syscall_handler(LINUX_X86_64_SYS_GETRANDOM, (long)rand_buf, 32, 0, 0, 0, 0);
    printf("getrandom(32) -> %ld\n", rand_ret);
    assert(rand_ret == 32);

    // 9. Verify uname
    struct linux_new_utsname uts;
    memset(&uts, 0, sizeof(uts));
    long uname_ret = linux_syscall_handler(LINUX_X86_64_SYS_UNAME, (long)&uts, 0, 0, 0, 0, 0);
    assert(uname_ret == 0);
    printf("uname sysname='%s', nodename='%s', release='%s'\n", uts.sysname, uts.nodename, uts.release);
    assert(strcmp(uts.sysname, "Linux") == 0);
    assert(strcmp(uts.nodename, "bharat-os") == 0);

    // 10. Verify arch_prctl (ARCH_SET_FS and ARCH_GET_FS)
    uint64_t target_fs = 0x7FFF12345000ULL;
    long fs_set = linux_syscall_handler(LINUX_X86_64_SYS_ARCH_PRCTL, 0x1002, (long)target_fs, 0, 0, 0, 0);
    assert(fs_set == 0);

    uint64_t retrieved_fs = 0;
    long fs_get = linux_syscall_handler(LINUX_X86_64_SYS_ARCH_PRCTL, 0x1003, (long)&retrieved_fs, 0, 0, 0, 0);
    assert(fs_get == 0);
    printf("arch_prctl retrieved FS base: 0x%lx (expected 0x%lx)\n", retrieved_fs, target_fs);
    assert(retrieved_fs == target_fs);

    // 11. Verify prlimit64
    linux_rlimit64_t lim;
    memset(&lim, 0, sizeof(lim));
    long prlim_ret = linux_syscall_handler(LINUX_X86_64_SYS_PRLIMIT64, 0, 3 /* RLIMIT_STACK */, 0, (long)&lim, 0, 0);
    assert(prlim_ret == 0);
    printf("RLIMIT_STACK cur: %lu, max: %lu\n", lim.rlim_cur, lim.rlim_max);
    assert(lim.rlim_cur == 8ULL * 1024ULL * 1024ULL);

    // 12. Verify futex (FUTEX_WAKE on empty bucket -> 0)
    uint32_t futex_val = 1;
    long futex_wake = linux_syscall_handler(LINUX_X86_64_SYS_FUTEX, (long)&futex_val, 1 /* FUTEX_WAKE */, 1, 0, 0, 0);
    printf("futex_wake -> %ld (expected 0)\n", futex_wake);
    assert(futex_wake == 0);

    // 13. Verify clone (with CLONE_THREAD)
    int parent_tid = 0;
    long clone_ret = linux_syscall_handler(LINUX_X86_64_SYS_CLONE, 0x00010000 | 0x00100000 /* CLONE_THREAD | CLONE_PARENT_SETTID */, 0x7FFF9000, (long)&parent_tid, 0, 0, 0);
    printf("clone -> tid %ld, parent_tid written: %d\n", clone_ret, parent_tid);
    assert(clone_ret == 42);
    assert(parent_tid == 42);

    // 14. Verify wait4 returns -ECHILD
    long wait_ret = linux_syscall_handler(LINUX_X86_64_SYS_WAIT4, -1, 0, 0, 0, 0, 0);
    assert(wait_ret == -LINUX_ECHILD);

    printf("\n[PASS] All Linux Personality Core system calls verified successfully!\n");
    return 0;
}
