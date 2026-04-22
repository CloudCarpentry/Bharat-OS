#include "compat/android/android_process.h"

#include "personality/translation_events.h"

int android_clone_task(android_process_desc_t* parent, uint32_t flags, android_process_desc_t** out_desc) {
    if (!out_desc) return -1;
    (void)parent;
    (void)flags;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

int android_futex_wait(uint32_t* uaddr, uint32_t val, uint64_t timeout_ns) {
    (void)uaddr;
    (void)val;
    (void)timeout_ns;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

int android_futex_wake(uint32_t* uaddr, uint32_t max_wake) {
    (void)uaddr;
    (void)max_wake;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return 0;
}

void* android_mmap(void* addr, uint64_t length, int prot, int flags, int fd, uint64_t offset) {
    (void)addr;
    (void)length;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_ENTER);
    bh_translation_event_record(BH_TRANSLATION_EVENT_FALLBACK);
    bh_translation_event_record(BH_TRANSLATION_EVENT_BOUNDARY_EXIT);
    return (void*)0;
}

int android_epoll_create(int size) {
    (void)size;
    return 0;
}

int android_epoll_ctl(int epfd, int op, int fd, void* event) {
    (void)epfd;
    (void)op;
    (void)fd;
    (void)event;
    return 0;
}

int android_epoll_wait(int epfd, void* events, int maxevents, int timeout) {
    (void)epfd;
    (void)events;
    (void)maxevents;
    (void)timeout;
    return 0;
}

int android_kill(uint32_t pid, int sig) {
    (void)pid;
    (void)sig;
    return 0;
}

int android_rt_sigaction(int signum, const void* act, void* oldact) {
    (void)signum;
    (void)act;
    (void)oldact;
    return 0;
}
