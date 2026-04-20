#include <lib/base/string.h>
#include <stdint.h>

#include <bharat/uapi/system/fault_domain.h>
#include "sched/sched.h"

// Simple mock implementation for V1
static uint64_t next_domain_id = 1;

int sys_fault_domain_create(const void* attr, uint64_t* out_domain) {
    if (!attr || !out_domain) return -1;
    // Basic validation
    bharat_fault_domain_attr_t local_attr;
    // We already verified the range in trap dispatch, but here we do a safe copy.
    // However trap_user_range_valid ensures it's readable. So a memcpy is safe.
    memcpy(&local_attr, attr, sizeof(bharat_fault_domain_attr_t));
    if (local_attr.version != 1) return -1;

    // Write out the result safely (range verified in trap.c)
    uint64_t new_id = next_domain_id++;
    memcpy(out_domain, &new_id, sizeof(uint64_t));
    return 0;
}

int sys_fault_domain_destroy(uint64_t domain) {
    // In V1, we don't really track them robustly for destruction
    return 0;
}

int sys_fault_domain_attach(uint64_t domain, uint64_t tid) {
    kthread_t *thread = sched_find_thread_by_id(tid);
    if (!thread) return -1;
    // TODO: store fault domain id
    (void)thread;
    return 0;
}
