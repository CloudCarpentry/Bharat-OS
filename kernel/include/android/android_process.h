#ifndef BHARAT_ANDROID_PROCESS_H
#define BHARAT_ANDROID_PROCESS_H

#include "android_personality.h"
#include "sched.h"

/*
 * Phase 1: Linux-Compatible Substrate Mapping
 *
 * Maps Android thread/process expectations to the Bharat-OS core.
 * Emphasizes single-home processes and multi-core threads.
 */

/**
 * @brief Android task/process personality tag.
 * Extends `kprocess_t` or `kthread_t` personality metadata.
 */
typedef struct {
    uint32_t process_id;           // Logical Process ID (PID)
    uint32_t home_core;            // Core authoritative for this process's MM/credentials

    // Android semantics map to Linux syscall base layers
    void* linux_fd_table_ref;      // Reference to the underlying Linux FD compatibility map
    void* linux_mm_authority_ref;  // Reference to memory mapping authority

    // Credentials & SELinux SIDs
    uint32_t selinux_sid;
    uint32_t euid;
    uint32_t egid;
} android_process_desc_t;

/**
 * @brief Clone and Thread Model abstraction.
 * Spawns a new Android thread or process, tying it to a home core or parent.
 *
 * @param parent Parent process or NULL for root processes (e.g. init).
 * @param flags  Clone flags determining sharing (VM, FS, files, etc.).
 * @param out_desc Pointer to the created Android process descriptor.
 * @return 0 on success, < 0 on error.
 */
int android_clone_task(android_process_desc_t* parent, uint32_t flags, android_process_desc_t** out_desc);

/**
 * @brief Futex and Wait Primitive mapping.
 * Uses the underlying Bharat-OS core `wait_queue_t` to emulate Android's Bionic futex semantics.
 */
int android_futex_wait(uint32_t* uaddr, uint32_t val, uint64_t timeout_ns);
int android_futex_wake(uint32_t* uaddr, uint32_t max_wake);

/**
 * @brief Memory Map / Shared Memory Contract.
 * Maps region requests to capability-backed multikernel memory objects.
 */
void* android_mmap(void* addr, uint64_t length, int prot, int flags, int fd, uint64_t offset);

/**
 * @brief Epoll/Poll Abstraction mapping.
 * Connects Android's event loop (Looper) to underlying capability event notifications.
 */
int android_epoll_create(int size);
int android_epoll_ctl(int epfd, int op, int fd, void* event);
int android_epoll_wait(int epfd, void* events, int maxevents, int timeout);

/**
 * @brief Signal Basics needed for Android userspace.
 */
int android_kill(uint32_t pid, int sig);
int android_rt_sigaction(int signum, const void* act, void* oldact);

#endif // BHARAT_ANDROID_PROCESS_H
