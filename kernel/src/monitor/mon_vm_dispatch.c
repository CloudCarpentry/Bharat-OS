#include "../../include/monitor/mon_vm_ops.h"
#include "../../include/mm/vm_space.h"
#include "../../include/mm/arch_vm.h"
#include "../../include/hal/hal.h"
#include <stddef.h>

int mon_vm_init(void) {
    // TODO: Initialize URPC channels for VM Monitor
    return 0;
}

static int handle_map(mon_vm_map_msg_t *msg) {
    if (!msg) return -1;
    // TODO: Walk canonical space, realize locally if needed
    // TODO: Send ACK if STRICT_ACK flag is set
    return 0;
}

static int handle_unmap(mon_vm_inv_msg_t *msg) {
    if (!msg) return -1;
    // TODO: Local invalidation, strict ack must be sent
    return 0;
}

static int handle_invalidate_range(mon_vm_inv_msg_t *msg) {
    if (!msg) return -1;
    // TODO: Send local TLB invalidation via arch_vm_ops
    return 0;
}

int mon_vm_dispatch(void *raw_msg, size_t len) {
    if (!raw_msg || len < sizeof(mon_vm_hdr_t)) return -1;

    mon_vm_hdr_t *hdr = (mon_vm_hdr_t *)raw_msg;
    switch (hdr->type) {
        case MON_VM_MAP:
            return handle_map((mon_vm_map_msg_t *)raw_msg);
        case MON_VM_UNMAP:
        case MON_VM_PROTECT:
            return handle_unmap((mon_vm_inv_msg_t *)raw_msg);
        case MON_VM_TLB_INVALIDATE_RANGE:
            return handle_invalidate_range((mon_vm_inv_msg_t *)raw_msg);
        case MON_VM_REALIZE:
            // TODO: Force realization on this core
            break;
        case MON_VM_PREPARE_RT:
            // TODO: Sched hint to prepare RT state
            break;
        case MON_VM_ACK:
            // TODO: Handle incoming ack to resolve pending synchronous operations
            break;
        default:
            return -1;
    }

    return 0;
}
