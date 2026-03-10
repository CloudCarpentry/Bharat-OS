#include <bharat/syscalls.h>

void bharat_exit(int code) {
    (void)code;
    while(1) {}
}

int bharat_write(int fd, const void* buf, size_t count) {
    (void)fd;
    (void)buf;
    return (int)count;
}

int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps) {
    if (storage_caps) {
        *storage_caps = 0U;
    }
    if (network_caps) {
        *network_caps = 0U;
    }
    return -38;
}

int bharat_sched_thread_create(void (*entry)(void), uint64_t* out_tid) {
    (void)entry;
    if (out_tid) {
        *out_tid = 0;
    }
    return -38;
}

int bharat_sched_thread_destroy(uint64_t tid) {
    (void)tid;
    return -38;
}

int bharat_sched_yield(void) {
    return 0;
}

int bharat_sched_sleep(uint64_t millis) {
    (void)millis;
    return 0;
}

int bharat_sched_set_priority(uint64_t tid, uint32_t priority) {
    (void)tid;
    (void)priority;
    return -38;
}

int bharat_sched_set_affinity(uint64_t tid, uint32_t affinity_mask) {
    (void)tid;
    (void)affinity_mask;
    return -38;
}
