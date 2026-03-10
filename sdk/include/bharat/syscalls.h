#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void bharat_exit(int code);
int bharat_write(int fd, const void* buf, size_t count);
int bharat_get_subsystem_caps(uint32_t* storage_caps, uint32_t* network_caps);

int bharat_sched_thread_create(void (*entry)(void), uint64_t* out_tid);
int bharat_sched_thread_destroy(uint64_t tid);
int bharat_sched_yield(void);
int bharat_sched_sleep(uint64_t millis);
int bharat_sched_set_priority(uint64_t tid, uint32_t priority);
int bharat_sched_set_affinity(uint64_t tid, uint32_t affinity_mask);

#ifdef __cplusplus
}
#endif
