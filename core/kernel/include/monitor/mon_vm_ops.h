#ifndef BHARAT_MON_VM_OPS_H
#define BHARAT_MON_VM_OPS_H

#include "mon_vm_proto.h"
#include "../mm/vm_space.h"
#include <stdbool.h>

// Initialize the VM Monitor subsystem
int mon_vm_init(void);

// Dispatch incoming URPC messages to the right handler
int mon_vm_dispatch(void *raw_msg, size_t len);

// Send a mapping update (map/protect/unmap)
int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict);
int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict);
int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict);

// Invalidation requests
int mon_vm_send_tlb_invalidate_range(vm_space_t *space, uintptr_t va, size_t len, bool strict);
int mon_vm_send_tlb_invalidate_all(vm_space_t *space, bool strict);

// RT readiness
int mon_vm_send_prepare_rt(vm_space_t *space, uint32_t target_core_id);

// Synchronously wait for ACKs on a strict transaction
int mon_vm_wait_for_acks(uint64_t txn_id, cpu_mask_t required_cores);

#endif // BHARAT_MON_VM_OPS_H
