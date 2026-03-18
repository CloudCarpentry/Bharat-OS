#include "../../include/monitor/mon_vm_ops.h"
#include "../../include/mm/vm_space.h"
#include "../../include/urpc/urpc_bootstrap.h"
#include <stddef.h>

int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict) {
    if (!space || !req) return -1;

    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_MAP;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = req->va;
    msg.pa_start = req->pa;
    msg.length = req->len;
    msg.prot = req->prot;
    msg.mem_type = req->mem_type;
    msg.map_flags = req->map_flags;

    // Dispatch via URPC...
    if (strict) {
        mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }
    return 0;
}

int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    if (!space) return -1;

    mon_vm_inv_msg_t msg = {0};
    msg.h.type = MON_VM_UNMAP;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;

    // Dispatch via URPC...
    if (strict) {
        mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }
    return 0;
}

int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict) {
    if (!space) return -1;

    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_PROTECT;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;
    msg.prot = prot;
    msg.mem_type = mem_type;

    // Dispatch via URPC...
    if (strict) {
        mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }
    return 0;
}

int mon_vm_send_tlb_invalidate_range(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    if (!space) return -1;

    mon_vm_inv_msg_t msg = {0};
    msg.h.type = MON_VM_TLB_INVALIDATE_RANGE;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;

    // Dispatch via URPC...
    if (strict) {
        mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }
    return 0;
}

int mon_vm_send_prepare_rt(vm_space_t *space, uint32_t target_core_id) {
    if (!space) return -1;

    mon_vm_hdr_t msg = {0};
    msg.type = MON_VM_PREPARE_RT;
    msg.space_id = space->space_id;
    msg.generation = space->generation;
    msg.flags = MON_VM_F_PREPARE_ONLY | MON_VM_F_STRICT_ACK;
    msg.dst_core = target_core_id;

    // Dispatch...
    return mon_vm_wait_for_acks(msg.txn_id, (1ULL << target_core_id));
}

int mon_vm_wait_for_acks(uint64_t txn_id, cpu_mask_t required_cores) {
    (void)txn_id;
    (void)required_cores;
    // Mock for now, would block on a wait queue until the monitor dispatch thread receives required ACKs
    return 0;
}
