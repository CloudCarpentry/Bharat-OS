#include "compat/android/android_process.h"
#include "compat/android/android_security.h"
#include <stddef.h>
#include <stdint.h>
#include "lib/base/string.h"

#include "personality/translation_events.h"

#define ANDROID_MAX_PROCESSES 64

static android_process_desc_t g_android_proc_pool[ANDROID_MAX_PROCESSES];
static int g_android_proc_used[ANDROID_MAX_PROCESSES];
static uint32_t g_next_pid = 1;

int android_clone_task(android_process_desc_t* parent, uint32_t flags, android_process_desc_t** out_desc) {
    if (!out_desc) {
        return -1;
    }

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);

    /* Allocate free slot in process descriptor pool */
    int slot = -1;
    for (size_t i = 0; i < ANDROID_MAX_PROCESSES; i++) {
        if (!g_android_proc_used[i]) {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -1; /* Process capacity reached */
    }

    android_process_desc_t* desc = &g_android_proc_pool[slot];
    memset(desc, 0, sizeof(*desc));

    if (parent) {
        desc->process_id = ++g_next_pid;
        desc->home_core = parent->home_core;
        desc->selinux_sid = parent->selinux_sid;
        desc->euid = parent->euid;
        desc->egid = parent->egid;
        desc->linux_fd_table_ref = parent->linux_fd_table_ref;
        desc->linux_mm_authority_ref = parent->linux_mm_authority_ref;
    } else {
        /* Root init daemon initialization */
        desc->process_id = 1;
        desc->home_core = 0;
        desc->selinux_sid = ANDROID_SEC_SID_INIT;
        desc->euid = 0;
        desc->egid = 0;
        desc->linux_fd_table_ref = NULL;
        desc->linux_mm_authority_ref = NULL;
    }

    (void)flags;
    g_android_proc_used[slot] = 1;
    *out_desc = desc;

    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

int android_futex_wait(uint32_t* uaddr, uint32_t val, uint64_t timeout_ns) {
    if (!uaddr) {
        return -1; /* -EFAULT */
    }

    (void)timeout_ns;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);

    /* Value check: atomic compare against expected value */
    if (*uaddr != val) {
        bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
        bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
        return -11; /* -EAGAIN */
    }

    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

int android_futex_wake(uint32_t* uaddr, uint32_t max_wake) {
    if (!uaddr) {
        return -1; /* -EFAULT */
    }

    (void)max_wake;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

static uint8_t g_android_mmap_backing[65536] __attribute__((aligned(4096)));

void* android_mmap(void* addr, uint64_t length, int prot, int flags, int fd, uint64_t offset) {
    if (length == 0 || length > sizeof(g_android_mmap_backing)) {
        return (void*)-1; /* MAP_FAILED */
    }

    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;

    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);

    return (void*)g_android_mmap_backing;
}

int android_epoll_create(int size) {
    if (size <= 0) {
        return -1; /* -EINVAL */
    }
    return 3; /* Synthetic epoll fd */
}

int android_epoll_ctl(int epfd, int op, int fd, void* event) {
    if (epfd < 0 || fd < 0) {
        return -1; /* -EBADF */
    }
    (void)op;
    (void)event;
    return 0;
}

int android_epoll_wait(int epfd, void* events, int maxevents, int timeout) {
    if (epfd < 0 || !events || maxevents <= 0) {
        return -1; /* -EINVAL */
    }
    (void)timeout;
    return 0;
}

int android_kill(uint32_t pid, int sig) {
    if (sig < 0 || sig > 64) {
        return -1; /* -EINVAL */
    }
    if (pid == 0) {
        return -1; /* -ESRCH */
    }
    return 0;
}

int android_rt_sigaction(int signum, const void* act, void* oldact) {
    if (signum <= 0 || signum > 64) {
        return -1; /* -EINVAL */
    }
    /* SIGKILL (9) and SIGSTOP (19) cannot be caught or modified */
    if (signum == 9 || signum == 19) {
        return -1; /* -EINVAL */
    }
    (void)act;
    (void)oldact;
    return 0;
}
