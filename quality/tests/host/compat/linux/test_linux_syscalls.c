#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "linux_compat.h"
#include "linux_errno.h"
#include "linux_futex.h"

// Mock for bh_translation_event_record
void bh_translation_event_record(int event) {
    (void)event;
}

static bh_process_t g_test_proc = { .process_id = 1 };
static bh_thread_t g_test_thread = { .thread_id = 1, .process = &g_test_proc };

bh_process_t *sched_current_process(void) { return &g_test_proc; }
bh_thread_t *sched_current_thread(void) { return &g_test_thread; }
uint64_t bh_ktime_now(void) { return 1000000000ULL; }
bh_status_t bh_copy_from_user(void *dst, const void *src, size_t n) { memcpy(dst, src, n); return BH_OK; }
bh_status_t bh_copy_to_user(void *dst, const void *src, size_t n) { memcpy(dst, src, n); return BH_OK; }
kstatus_t bh_status_to_kstatus(bh_status_t st) { return (kstatus_t)st; }
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
kstatus_t sched_enqueue(bh_thread_t *thread, uint32_t core) { (void)thread; (void)core; return K_OK; }
void sched_sleep(uint64_t ms) { (void)ms; }
void sched_wakeup(bh_thread_t *th) { (void)th; }
void bh_thread_yield(void) {}
bh_operation_result_t bh_sys_read(bh_syscall_ctx_t *ctx) { (void)ctx; return bh_op_result_value(-38); }
bh_operation_result_t bh_sys_write(bh_syscall_ctx_t *ctx) { return bh_op_result_value((long)ctx->regs.arg[2]); }
bh_operation_result_t bh_sys_thread_exit(bh_syscall_ctx_t *ctx) { (void)ctx; return bh_op_result_value(0); }

int main() {
    printf("Running test_linux_syscalls...\n");

    subsys_instance_t env;
    env.type = SUBSYS_TYPE_LINUX;
    env.is_running = 1;
    env.memory_limit_mb = 1024;
    env.cpu_core_allocation_mask = 1;

    assert(linux_subsys_init(&env) == 0);

    // Test getpid (39)
    long ret = linux_syscall_handler(39, 0, 0, 0, 0, 0, 0);
    assert(ret == 1);

    // Test open (2)
    long fd = linux_syscall_handler(2, 0, 0, 0, 0, 0, 0);
    assert(fd >= 3);

    // Test read (0) on console
    ret = linux_syscall_handler(0, 0, 0, 0, 0, 0, 0);
    assert(ret == -38); // Console fallback

    // Test write (1) on console
    ret = linux_syscall_handler(1, 1, 0, 10, 0, 0, 0);
    assert(ret == 10);

    // Test close (3)
    ret = linux_syscall_handler(3, fd, 0, 0, 0, 0, 0);
    assert(ret == 0);

    printf("test_linux_syscalls passed!\n");
    return 0;
}
