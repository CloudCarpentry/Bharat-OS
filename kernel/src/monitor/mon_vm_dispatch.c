#include "../../include/monitor/mon_vm_ops.h"
#include "../../include/mm/vm_space.h"
#include "../../include/mm/arch_vm.h"
#include "../../include/hal/hal.h"
#include "mon_vm_state.h"
#include <stddef.h>

static void send_ack(const mon_vm_hdr_t *hdr, int32_t status) {
    if (!(hdr->flags & MON_VM_F_STRICT_ACK)) return;

    // Construct ACK message
    mon_vm_ack_msg_t ack = {0};
    ack.h.type = MON_VM_ACK;
    ack.h.txn_id = hdr->txn_id;
    ack.h.space_id = hdr->space_id;
    ack.h.generation = hdr->generation;
    ack.h.dst_core = hdr->src_core;
    ack.h.src_core = hal_cpu_get_id();
    ack.status = status;
    ack.realize_state = 0;

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_acks_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    // Mock dispatch via URPC
    // urpc_bootstrap_send(ack.h.dst_core, (uint64_t)&ack);
}

static int handle_map(mon_vm_map_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_map_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF) || (msg->pa_start & 0xFFF)) {
        send_ack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    // Call underlying arch logic here (stubbed for now)
    // Per user instruction, do not silently succeed on a stub path.
    int res = MON_VM_STATUS_UNSUPPORTED;

    send_ack(&msg->h, res);
    return res;
}

static int handle_unmap(mon_vm_inv_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_inv_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF)) {
        send_ack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    // Call underlying arch logic here (stubbed for now)
    // Per user instruction, do not silently succeed on a stub path.
    int res = MON_VM_STATUS_UNSUPPORTED;

    send_ack(&msg->h, res);
    return res;
}

static int handle_invalidate_range(mon_vm_inv_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_inv_msg_t)) return MON_VM_STATUS_MALFORMED;
    if (msg->length == 0 || (msg->va_start & 0xFFF)) {
        send_ack(&msg->h, MON_VM_STATUS_MALFORMED);
        return MON_VM_STATUS_MALFORMED;
    }

    // Call underlying arch logic here (stubbed for now)
    // Per user instruction, do not silently succeed on a stub path.
    int res = MON_VM_STATUS_UNSUPPORTED;

    send_ack(&msg->h, res);
    return res;
}

static int handle_ack(mon_vm_ack_msg_t *msg, size_t len) {
    if (len < sizeof(mon_vm_ack_msg_t)) return MON_VM_STATUS_MALFORMED;

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_acks_received++;

    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        if (g_mon_vm_state.pending_txns[i].in_use && g_mon_vm_state.pending_txns[i].txn_id == msg->h.txn_id) {
            mon_vm_pending_txn_t *txn = &g_mon_vm_state.pending_txns[i];

            // Record ACK from the source core
            txn->received_acks_mask |= (1ULL << msg->h.src_core);

            if (msg->status != MON_VM_STATUS_SUCCESS) {
                txn->final_status = msg->status;
            }
            break;
        }
    }
    spinlock_release(&g_mon_vm_state.lock);

    return 0;
}

int mon_vm_dispatch(void *raw_msg, size_t len) {
    if (!raw_msg || len < sizeof(mon_vm_hdr_t)) return MON_VM_STATUS_MALFORMED;

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_received++;
    spinlock_release(&g_mon_vm_state.lock);

    mon_vm_hdr_t *hdr = (mon_vm_hdr_t *)raw_msg;
    switch (hdr->type) {
        case MON_VM_MAP:
            return handle_map((mon_vm_map_msg_t *)raw_msg, len);
        case MON_VM_UNMAP:
        case MON_VM_PROTECT:
            return handle_unmap((mon_vm_inv_msg_t *)raw_msg, len);
        case MON_VM_TLB_INVALIDATE_RANGE:
            return handle_invalidate_range((mon_vm_inv_msg_t *)raw_msg, len);
        case MON_VM_REALIZE:
            send_ack(hdr, MON_VM_STATUS_UNSUPPORTED);
            return MON_VM_STATUS_UNSUPPORTED;
        case MON_VM_PREPARE_RT:
            send_ack(hdr, MON_VM_STATUS_UNSUPPORTED);
            return MON_VM_STATUS_UNSUPPORTED;
        case MON_VM_ACK:
        case MON_VM_NACK:
            return handle_ack((mon_vm_ack_msg_t *)raw_msg, len);
        default:
            send_ack(hdr, MON_VM_STATUS_UNSUPPORTED);
            return MON_VM_STATUS_UNSUPPORTED;
    }

    return 0;
}
