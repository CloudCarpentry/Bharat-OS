#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "linux_compat.h"
#include "linux_errno.h"
#include "linux_types.h"
#include "linux_futex.h"
#include "linux_pipe.h"
#include "linux_eventfd.h"
#include "linux_timerfd.h"
#include "linux_epoll.h"
#include "linux_procfs.h"
#include "linux_personality.h"
#include "linux_syscall_numbers_x86_64.h"

// Mock kernel structures
static bh_process_t g_test_proc;
static bh_thread_t g_test_thread;
static uint64_t g_mock_time_ns = 1000000000ULL;

bh_process_t *sched_current_process(void) { return &g_test_proc; }
bh_thread_t *sched_current_thread(void) { return &g_test_thread; }
uint64_t bh_ktime_now(void) { return g_mock_time_ns; }
void sched_sleep(uint64_t ms) { (void)ms; }
void sched_wakeup(bh_thread_t *th) { (void)th; }
void bh_thread_yield(void) {}

long bh_syscall_gate(trap_frame_t *frame, const trap_info_t *info) { (void)frame; (void)info; return 0; }
kstatus_t vm_map_region(address_space_t *as, const vm_map_request_t *req, uintptr_t *out) { (void)as; (void)req; if (out) *out = 0x7FFF0000; return K_OK; }
kstatus_t vm_protect_region(address_space_t *as, uintptr_t addr, size_t len, uint32_t prot) { (void)as; (void)addr; (void)len; (void)prot; return K_OK; }
kstatus_t vm_unmap_region(address_space_t *as, uintptr_t addr, size_t len) { (void)as; (void)addr; (void)len; return K_OK; }

static bh_thread_t g_spawned_thread;
bh_thread_t *thread_create_detached(bh_process_t *parent, void (*entry_point)(void)) {
    (void)entry_point;
    memset(&g_spawned_thread, 0, sizeof(g_spawned_thread));
    g_spawned_thread.thread_id = 99;
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

int main(void) {
    printf("=== Starting Bharat-OS Linux Personality Phase 4 Ecosystem Verification ===\n\n");

    subsys_instance_t env;
    memset(&env, 0, sizeof(env));
    env.type = SUBSYS_TYPE_LINUX;
    env.is_running = 1;

    assert(linux_subsys_init(&env) == 0);

    g_test_proc.process_id = 2000;
    g_test_thread.thread_id = 2001;
    g_test_thread.process = &g_test_proc;

    // ─────────────────────────────────────────────────────────────
    // 1. Pipe & Pipe2 Verification
    // ─────────────────────────────────────────────────────────────
    printf("[1/6] Verifying pipe2() lifecycle, buffered data transfer, EOF, and EPIPE...\n");
    int pipefd[2] = {-1, -1};
    long ret = linux_syscall_handler(LINUX_X86_64_SYS_PIPE2, (long)pipefd, LINUX_O_NONBLOCK | LINUX_O_CLOEXEC, 0, 0, 0, 0);
    assert(ret == 0);
    assert(pipefd[0] >= 3 && pipefd[1] >= 3 && pipefd[0] != pipefd[1]);
    printf("  pipe2 created: read_fd=%d, write_fd=%d\n", pipefd[0], pipefd[1]);

    // Non-blocking read on empty pipe should return -EAGAIN
    char read_buf[128];
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, pipefd[0], (long)read_buf, sizeof(read_buf), 0, 0, 0);
    assert(ret == -LINUX_EAGAIN);
    printf("  empty pipe non-blocking read -> -EAGAIN (ok)\n");

    // Write data into pipe
    const char *msg = "Bharat-OS Microkernel Pipe Message";
    size_t msg_len = strlen(msg);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_WRITE, pipefd[1], (long)msg, msg_len, 0, 0, 0);
    assert(ret == (long)msg_len);
    printf("  pipe write(%zu bytes) -> %ld\n", msg_len, ret);

    // Read back data from pipe
    memset(read_buf, 0, sizeof(read_buf));
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, pipefd[0], (long)read_buf, sizeof(read_buf), 0, 0, 0);
    assert(ret == (long)msg_len);
    assert(strcmp(read_buf, msg) == 0);
    printf("  pipe read back '%s' -> matches exactly\n", read_buf);

    // Close write end -> EOF
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, pipefd[1], 0, 0, 0, 0, 0) == 0);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, pipefd[0], (long)read_buf, sizeof(read_buf), 0, 0, 0);
    assert(ret == 0);
    printf("  read after write-end close -> 0 (EOF ok)\n");

    // Close read end -> create another pipe to test EPIPE
    int pipefd2[2];
    assert(linux_syscall_handler(LINUX_X86_64_SYS_PIPE2, (long)pipefd2, 0, 0, 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, pipefd2[0], 0, 0, 0, 0, 0) == 0);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_WRITE, pipefd2[1], (long)"fail", 4, 0, 0, 0);
    assert(ret == -LINUX_EPIPE);
    printf("  write after read-end close -> -EPIPE (broken pipe ok)\n");
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, pipefd2[1], 0, 0, 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, pipefd[0], 0, 0, 0, 0, 0) == 0);

    // ─────────────────────────────────────────────────────────────
    // 2. Eventfd & Eventfd2 Verification
    // ─────────────────────────────────────────────────────────────
    printf("\n[2/6] Verifying eventfd2() counter accumulation and EFD_SEMAPHORE...\n");
    long efd = linux_syscall_handler(LINUX_X86_64_SYS_EVENTFD2, 5, LINUX_EFD_NONBLOCK, 0, 0, 0, 0);
    assert(efd >= 3);
    printf("  eventfd2 created with initval=5: fd=%ld\n", efd);

    // Write 10 to counter
    uint64_t val = 10;
    ret = linux_syscall_handler(LINUX_X86_64_SYS_WRITE, efd, (long)&val, sizeof(val), 0, 0, 0);
    assert(ret == 8);

    // Read counter (default: returns and resets accumulated total: 5 + 10 = 15)
    uint64_t out_val = 0;
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, efd, (long)&out_val, sizeof(out_val), 0, 0, 0);
    assert(ret == 8);
    assert(out_val == 15);
    printf("  eventfd read counter -> %lu (expected 15)\n", out_val);

    // Subsequent read returns -EAGAIN because counter is now 0
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, efd, (long)&out_val, sizeof(out_val), 0, 0, 0);
    assert(ret == -LINUX_EAGAIN);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, efd, 0, 0, 0, 0, 0) == 0);

    // Semaphore mode: each read decrements counter by 1
    long efd_sem = linux_syscall_handler(LINUX_X86_64_SYS_EVENTFD2, 2, LINUX_EFD_SEMAPHORE | LINUX_EFD_NONBLOCK, 0, 0, 0, 0);
    assert(efd_sem >= 3);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, efd_sem, (long)&out_val, sizeof(out_val), 0, 0, 0);
    assert(ret == 8 && out_val == 1);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, efd_sem, (long)&out_val, sizeof(out_val), 0, 0, 0);
    assert(ret == 8 && out_val == 1);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, efd_sem, (long)&out_val, sizeof(out_val), 0, 0, 0);
    assert(ret == -LINUX_EAGAIN);
    printf("  EFD_SEMAPHORE mode decrements by 1 until empty (ok)\n");
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, efd_sem, 0, 0, 0, 0, 0) == 0);

    // ─────────────────────────────────────────────────────────────
    // 3. Timerfd Verification
    // ─────────────────────────────────────────────────────────────
    printf("\n[3/6] Verifying timerfd_create, timerfd_settime, and expiration ticks...\n");
    long tfd = linux_syscall_handler(LINUX_X86_64_SYS_TIMERFD_CREATE, 1 /* CLOCK_MONOTONIC */, LINUX_TFD_NONBLOCK, 0, 0, 0, 0);
    assert(tfd >= 3);
    printf("  timerfd_create -> fd %ld\n", tfd);

    // Arm timer: initial 1s (1000000000 ns), periodic 500ms (500000000 ns)
    linux_itimerspec_t its;
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 500000000;
    ret = linux_syscall_handler(LINUX_X86_64_SYS_TIMERFD_SETTIME, tfd, 0, (long)&its, 0, 0, 0);
    assert(ret == 0);

    // Reading immediately when clock has not advanced -> -EAGAIN
    uint64_t expirations = 0;
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, tfd, (long)&expirations, sizeof(expirations), 0, 0, 0);
    assert(ret == -LINUX_EAGAIN);
    printf("  timerfd before expiry -> -EAGAIN (ok)\n");

    // Advance mock time by 2.2 seconds (initial 1s + 2 intervals of 0.5s = 3 expirations)
    g_mock_time_ns += 2200000000ULL;
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, tfd, (long)&expirations, sizeof(expirations), 0, 0, 0);
    assert(ret == 8);
    assert(expirations >= 2);
    printf("  timerfd after advancing time -> %lu expirations read (ok)\n", expirations);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, tfd, 0, 0, 0, 0, 0) == 0);

    // ─────────────────────────────────────────────────────────────
    // 4. Epoll (epoll_create1, ctl, wait) Verification
    // ─────────────────────────────────────────────────────────────
    printf("\n[4/6] Verifying epoll_create1(), epoll_ctl(), and epoll_wait() multiplexing...\n");
    long epfd = linux_syscall_handler(LINUX_X86_64_SYS_EPOLL_CREATE1, LINUX_EPOLL_CLOEXEC, 0, 0, 0, 0, 0);
    assert(epfd >= 3);
    printf("  epoll_create1 -> epfd %ld\n", epfd);

    // Create a pipe and register read-end with epoll
    int poll_pipe[2];
    assert(linux_syscall_handler(LINUX_X86_64_SYS_PIPE2, (long)poll_pipe, LINUX_O_NONBLOCK, 0, 0, 0, 0) == 0);

    struct linux_epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = LINUX_EPOLLIN;
    ev.data.u64 = 0xCAFEULL; // user cookie
    ret = linux_syscall_handler(LINUX_X86_64_SYS_EPOLL_CTL, epfd, LINUX_EPOLL_CTL_ADD, poll_pipe[0], (long)&ev, 0, 0);
    assert(ret == 0);

    // epoll_wait on empty pipe -> 0 events
    struct linux_epoll_event ready_events[4];
    memset(ready_events, 0, sizeof(ready_events));
    ret = linux_syscall_handler(LINUX_X86_64_SYS_EPOLL_WAIT, epfd, (long)ready_events, 4, 0 /* 0 timeout */, 0, 0);
    assert(ret == 0);
    printf("  epoll_wait on empty pipe -> 0 events ready\n");

    // Write to pipe
    assert(linux_syscall_handler(LINUX_X86_64_SYS_WRITE, poll_pipe[1], (long)"data", 4, 0, 0, 0) == 4);

    // epoll_wait now returns 1 ready event
    ret = linux_syscall_handler(LINUX_X86_64_SYS_EPOLL_WAIT, epfd, (long)ready_events, 4, 0, 0, 0);
    assert(ret == 1);
    assert(ready_events[0].data.u64 == 0xCAFEULL);
    assert(ready_events[0].events & LINUX_EPOLLIN);
    printf("  epoll_wait after pipe write -> 1 event ready (data cookie=0x%lx, events=0x%x)\n", (unsigned long)ready_events[0].data.u64, ready_events[0].events);

    // Clean up epoll & pipe
    assert(linux_syscall_handler(LINUX_X86_64_SYS_EPOLL_CTL, epfd, LINUX_EPOLL_CTL_DEL, poll_pipe[0], 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, epfd, 0, 0, 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, poll_pipe[0], 0, 0, 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, poll_pipe[1], 0, 0, 0, 0, 0) == 0);

    // ─────────────────────────────────────────────────────────────
    // 5. Procfs Virtual Filesystem Verification
    // ─────────────────────────────────────────────────────────────
    printf("\n[5/6] Verifying virtual /proc files (/proc/self/status, /proc/cpuinfo, /proc/meminfo, /proc/version)...\n");
    const char *proc_files[] = {
        "/proc/self/status",
        "/proc/cpuinfo",
        "/proc/meminfo",
        "/proc/version",
        "/proc/mounts"
    };
    for (int i = 0; i < 5; i++) {
        long pfd = linux_syscall_handler(LINUX_X86_64_SYS_OPENAT, 0, (long)proc_files[i], 0, 0, 0, 0);
        assert(pfd >= 3);
        char pbuf[256] = {0};
        ret = linux_syscall_handler(LINUX_X86_64_SYS_READ, pfd, (long)pbuf, sizeof(pbuf) - 1, 0, 0, 0);
        assert(ret > 0);
        printf("  %s -> %ld bytes: '%.40s...'\n", proc_files[i], ret, pbuf);
        assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, pfd, 0, 0, 0, 0, 0) == 0);
    }

    // ─────────────────────────────────────────────────────────────
    // 6. Working Directory, Access, Stat, Fcntl, Dup & Fork Verification
    // ─────────────────────────────────────────────────────────────
    printf("\n[6/6] Verifying getcwd, chdir, access, fcntl, dup2/3, and process fork...\n");
    char cwd_buf[128];
    ret = linux_syscall_handler(LINUX_X86_64_SYS_GETCWD, (long)cwd_buf, sizeof(cwd_buf), 0, 0, 0, 0);
    assert(ret > 0);
    assert(strcmp(cwd_buf, "/") == 0);
    printf("  initial getcwd -> '%s'\n", cwd_buf);

    ret = linux_syscall_handler(LINUX_X86_64_SYS_CHDIR, (long)"/usr/bin", 0, 0, 0, 0, 0);
    assert(ret == 0);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_GETCWD, (long)cwd_buf, sizeof(cwd_buf), 0, 0, 0, 0);
    assert(ret > 0);
    assert(strcmp(cwd_buf, "/usr/bin") == 0);
    printf("  after chdir -> '%s'\n", cwd_buf);

    // access() on existing /proc file returns 0
    ret = linux_syscall_handler(LINUX_X86_64_SYS_ACCESS, (long)"/proc/version", 0, 0, 0, 0, 0);
    assert(ret == 0);
    printf("  access(/proc/version) -> 0 (accessible)\n");

    // readlink on /proc/self/exe
    char link_buf[64] = {0};
    ret = linux_syscall_handler(LINUX_X86_64_SYS_READLINK, (long)"/proc/self/exe", (long)link_buf, sizeof(link_buf), 0, 0, 0);
    assert(ret > 0);
    printf("  readlink(/proc/self/exe) -> '%s'\n", link_buf);

    // dup2 & dup3
    long devzero = linux_syscall_handler(LINUX_X86_64_SYS_OPENAT, 0, (long)"/dev/zero", 0, 0, 0, 0);
    assert(devzero >= 3);
    ret = linux_syscall_handler(LINUX_X86_64_SYS_DUP2, devzero, 15, 0, 0, 0, 0);
    assert(ret == 15);
    // fcntl F_GETFL
    ret = linux_syscall_handler(LINUX_X86_64_SYS_FCNTL, 15, 3 /* F_GETFL */, 0, 0, 0, 0);
    assert(ret >= 0);
    // fcntl F_SETFL to non-blocking
    ret = linux_syscall_handler(LINUX_X86_64_SYS_FCNTL, 15, 4 /* F_SETFL */, LINUX_O_NONBLOCK, 0, 0, 0);
    assert(ret == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, 15, 0, 0, 0, 0, 0) == 0);
    assert(linux_syscall_handler(LINUX_X86_64_SYS_CLOSE, devzero, 0, 0, 0, 0, 0) == 0);

    // Process Fork
    long child_pid = linux_syscall_handler(LINUX_X86_64_SYS_FORK, 0, 0, 0, 0, 0, 0);
    printf("  fork() -> child PID %ld created\n", child_pid);
    assert(child_pid > 0);

    // Execve stub returns 0
    ret = linux_syscall_handler(LINUX_X86_64_SYS_EXECVE, (long)"/bin/sh", 0, 0, 0, 0, 0);
    assert(ret == 0);
    printf("  execve('/bin/sh') -> 0 (success)\n");

    printf("\n====================================================================\n");
    printf("[PASS] All Phase 4 Linux Ecosystem Features Verified Successfully!\n");
    printf("====================================================================\n");

    return 0;
}
