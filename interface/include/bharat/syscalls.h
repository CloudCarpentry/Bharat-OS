#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void bharat_exit(int code);
int bharat_read(int fd, void* buf, size_t count);
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
int bharat_intent_set(uint64_t tid, const void* intent);
int bharat_intent_get(uint64_t tid, void* intent);

int bharat_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr);

int bharat_fault_domain_create(const void* attr, uint64_t* out_domain);
int bharat_fault_domain_destroy(uint64_t domain);
int bharat_fault_domain_attach(uint64_t domain, uint64_t tid);
