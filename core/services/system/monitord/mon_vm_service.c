#include "../../include/monitor/mon_vm_ops.h"
#include "../../include/mm/vm_space.h"
#include "../../include/urpc/urpc_bootstrap.h"
#include "../../include/hal/hal.h"
#include "../../include/bharat/cpu_local.h"
#include "mon_vm_state.h"
#include <stddef.h>

int mon_vm_init(void) {
    if (g_mon_vm_state.initialized) {
        return 0; // Already initialized
    }

    spin_lock_init(&g_mon_vm_state.lock);

    // Bind the URPC endpoints if not already done,
    // relying on urpc_bootstrap_core mechanism here or similar.
    // Ensure channel state for communication is viable by asserting
    // the underlying urpc channels exist. In a multikernel setup,
    // this would establish monitor queues to peer cores.
    uint32_t my_core = hal_cpu_get_id();
    if (!urpc_is_ready(my_core)) {
        // Attempt to bind/bootstrap if possible or fail closed
        if (urpc_bootstrap_core(my_core) != 0) {
            return MON_VM_STATUS_CHANNEL_ERROR;
        }
    }

    g_mon_vm_state.next_txn_id = 1;
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        g_mon_vm_state.pending_txns[i].in_use = false;
    }

    g_mon_vm_state.stat_requests_sent = 0;
    g_mon_vm_state.stat_requests_received = 0;
    g_mon_vm_state.stat_acks_sent = 0;
    g_mon_vm_state.stat_acks_received = 0;
    g_mon_vm_state.stat_timeouts = 0;
    g_mon_vm_state.stat_failures = 0;

    g_mon_vm_state.initialized = true;
    return 0;
}

static mon_vm_pending_txn_t* alloc_txn(uint64_t *out_txn_id) {
    spinlock_acquire(&g_mon_vm_state.lock);
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        if (!g_mon_vm_state.pending_txns[i].in_use) {
            g_mon_vm_state.pending_txns[i].in_use = true;
            g_mon_vm_state.pending_txns[i].txn_id = g_mon_vm_state.next_txn_id++;
            g_mon_vm_state.pending_txns[i].expected_acks_mask = 0;
            g_mon_vm_state.pending_txns[i].received_acks_mask = 0;
            g_mon_vm_state.pending_txns[i].final_status = MON_VM_STATUS_SUCCESS;
            *out_txn_id = g_mon_vm_state.pending_txns[i].txn_id;
            spinlock_release(&g_mon_vm_state.lock);
            return &g_mon_vm_state.pending_txns[i];
        }
    }
    spinlock_release(&g_mon_vm_state.lock);
    return NULL;
}

static void free_txn(mon_vm_pending_txn_t *txn) {
    spinlock_acquire(&g_mon_vm_state.lock);
    txn->in_use = false;
    spinlock_release(&g_mon_vm_state.lock);
}

int mon_vm_send_map(vm_space_t *space, const vm_map_req_t *req, bool strict) {
    if (!space || !req) return MON_VM_STATUS_MALFORMED;
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    uint64_t txn_id = 0;
    mon_vm_pending_txn_t *txn = alloc_txn(&txn_id);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_MAP;
    msg.h.txn_id = txn_id;
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
    for (uint32_t core = 0; core < MAX_CPUS; core++) {
        if (space->active_cores & (1ULL << core)) {
            urpc_bootstrap_send(core, (uint64_t)&msg); // Pass pointer in payload for MVP
        }
    }

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    if (strict) {
        return mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }

    free_txn(txn);
    return MON_VM_STATUS_SUCCESS;
}

int mon_vm_send_unmap(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    if (!space) return MON_VM_STATUS_MALFORMED;
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    uint64_t txn_id = 0;
    mon_vm_pending_txn_t *txn = alloc_txn(&txn_id);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_inv_msg_t msg = {0};
    msg.h.type = MON_VM_UNMAP;
    msg.h.txn_id = txn_id;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;

    // Dispatch via URPC...
    for (uint32_t core = 0; core < MAX_CPUS; core++) {
        if (space->active_cores & (1ULL << core)) {
            urpc_bootstrap_send(core, (uint64_t)&msg);
        }
    }

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    if (strict) {
        return mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }

    free_txn(txn);
    return MON_VM_STATUS_SUCCESS;
}

int mon_vm_send_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type, bool strict) {
    if (!space) return MON_VM_STATUS_MALFORMED;
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    uint64_t txn_id = 0;
    mon_vm_pending_txn_t *txn = alloc_txn(&txn_id);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_map_msg_t msg = {0};
    msg.h.type = MON_VM_PROTECT;
    msg.h.txn_id = txn_id;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;
    msg.prot = prot;
    msg.mem_type = mem_type;

    // Dispatch via URPC...
    for (uint32_t core = 0; core < MAX_CPUS; core++) {
        if (space->active_cores & (1ULL << core)) {
            urpc_bootstrap_send(core, (uint64_t)&msg);
        }
    }

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    if (strict) {
        return mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }

    free_txn(txn);
    return MON_VM_STATUS_SUCCESS;
}

int mon_vm_send_tlb_invalidate_range(vm_space_t *space, uintptr_t va, size_t len, bool strict) {
    if (!space) return MON_VM_STATUS_MALFORMED;
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    uint64_t txn_id = 0;
    mon_vm_pending_txn_t *txn = alloc_txn(&txn_id);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_inv_msg_t msg = {0};
    msg.h.type = MON_VM_TLB_INVALIDATE_RANGE;
    msg.h.txn_id = txn_id;
    msg.h.space_id = space->space_id;
    msg.h.generation = space->generation;
    msg.h.flags = strict ? MON_VM_F_STRICT_ACK : 0;

    msg.va_start = va;
    msg.length = len;

    // Dispatch via URPC...
    for (uint32_t core = 0; core < MAX_CPUS; core++) {
        if (space->active_cores & (1ULL << core)) {
            urpc_bootstrap_send(core, (uint64_t)&msg);
        }
    }

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    if (strict) {
        return mon_vm_wait_for_acks(msg.h.txn_id, space->active_cores);
    }

    free_txn(txn);
    return MON_VM_STATUS_SUCCESS;
}

int mon_vm_send_prepare_rt(vm_space_t *space, uint32_t target_core_id) {
    if (!space) return MON_VM_STATUS_MALFORMED;
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    uint64_t txn_id = 0;
    mon_vm_pending_txn_t *txn = alloc_txn(&txn_id);
    if (!txn) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_hdr_t msg = {0};
    msg.type = MON_VM_PREPARE_RT;
    msg.txn_id = txn_id;
    msg.space_id = space->space_id;
    msg.generation = space->generation;
    msg.flags = MON_VM_F_PREPARE_ONLY | MON_VM_F_STRICT_ACK;
    msg.dst_core = target_core_id;

    // Dispatch...
    urpc_bootstrap_send(target_core_id, (uint64_t)&msg);

    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_requests_sent++;
    spinlock_release(&g_mon_vm_state.lock);

    return mon_vm_wait_for_acks(msg.txn_id, (1ULL << target_core_id));
}

// Dummy timeout constant
#define MON_VM_TIMEOUT_SPINS 100000

int mon_vm_wait_for_acks(uint64_t txn_id, cpu_mask_t required_cores) {
    if (!g_mon_vm_state.initialized) return MON_VM_STATUS_CHANNEL_ERROR;

    mon_vm_pending_txn_t *target_txn = NULL;

    spinlock_acquire(&g_mon_vm_state.lock);
    for (int i = 0; i < MAX_PENDING_TXNS; i++) {
        if (g_mon_vm_state.pending_txns[i].in_use && g_mon_vm_state.pending_txns[i].txn_id == txn_id) {
            target_txn = &g_mon_vm_state.pending_txns[i];
            break;
        }
    }
    spinlock_release(&g_mon_vm_state.lock);

    if (!target_txn) return MON_VM_STATUS_MALFORMED; // Invalid txn id

    target_txn->expected_acks_mask = required_cores;

    // In a real RTOS this would sleep/yield or block on a wait queue.
    // For now, we spin up to a timeout.
    uint32_t spins = 0;
    while (spins < MON_VM_TIMEOUT_SPINS) {
        // Assume an incoming message IRQ or poll processes the queue and updates
        // received_acks_mask and final_status. We yield to allow scheduling.
        hal_core_poll_event(); // Yields or relaxes

        spinlock_acquire(&g_mon_vm_state.lock);
        bool done = ((target_txn->received_acks_mask & target_txn->expected_acks_mask) == target_txn->expected_acks_mask)
                    || (target_txn->final_status != MON_VM_STATUS_SUCCESS);
        int32_t status = target_txn->final_status;
        spinlock_release(&g_mon_vm_state.lock);

        if (done) {
            free_txn(target_txn);
            return status;
        }
        spins++;
    }

    // Timeout hit
    spinlock_acquire(&g_mon_vm_state.lock);
    g_mon_vm_state.stat_timeouts++;
    target_txn->final_status = MON_VM_STATUS_TIMEOUT;
    spinlock_release(&g_mon_vm_state.lock);

    free_txn(target_txn);
    return MON_VM_STATUS_TIMEOUT;
}
