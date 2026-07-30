#include "../../../../include/mm/vm_space.h"
#include "../../../../include/mm/vm_mapping.h"
#include "../../../../include/monitor/mon_vm_ops.h"
#include "../../../../include/hal/hal.h"
#include "../../../../include/mm.h"
#include "../../../../include/slab.h"
#include "../../../../include/spinlock.h"
#include "../../../../include/kernel_safety.h"
#include "../../../../include/multicore.h"
#include "../../../../include/mm/arch_vm.h"
#include "../../../../include/mm/prot_domain.h"
#include "../../../monitor/mon_vm_state.h"
#include <stddef.h>

#ifndef hal_get_core_id
extern uint32_t hal_get_core_id(void);
#endif

#ifndef spinlock_acquire
#define spinlock_acquire spin_lock
#endif
#ifndef spinlock_release
#define spinlock_release spin_unlock
#endif

// Forward declaration of destroy message handler types
#define MON_VM_SPACE_DESTROY 0x1001

// Helper to start a per-space mutation transaction under lock
static int vm_mutation_begin(vm_space_t *space, vm_mutation_kind_t kind) {
    spinlock_acquire(&space->lock);

    // Reject operations if address space is in transition/fault state
    if (space->aspace && (space->aspace->state == ASPACE_STATE_DYING || space->aspace->state == ASPACE_STATE_POISONED)) {
        spinlock_release(&space->lock);
        return -1;
    }

    // Check if another mutation is already active on this space
    if (space->mutation_kind != VM_MUTATION_NONE) {
        spinlock_release(&space->lock);
        return MON_VM_STATUS_BUSY;
    }

    space->mutation_kind = kind;
    return 0; // Lock is still HELD after success, so caller can snapshot safely!
}

// Helper to finish a per-space mutation transaction and release the lock
static void vm_mutation_finish(vm_space_t *space) {
    space->mutation_kind = VM_MUTATION_NONE;
    spinlock_release(&space->lock);
}

int vm_map(vm_space_t *space, const vm_map_req_t *req) {
    if (!space || !req) return -1;

    // Begin mutation transaction (acquires space->lock on success)
    int begin_ret = vm_mutation_begin(space, VM_MUTATION_MAP);
    if (begin_ret != 0) return begin_ret;

    address_space_t *aspace = space->aspace;

    // Create physical backing object to retain physical base address survives
    vm_object_t *phys_obj = vm_object_create_device(req->pa, req->len, req->mem_type, 0);
    if (!phys_obj) {
        vm_mutation_finish(space);
        return -1;
    }

    // Attach canonical region staging (do not publish yet but allocate)
    vm_region_t *r = NULL;
    int attach_ret = aspace_region_attach(aspace, req->va, req->len, req->prot, req->map_flags, VM_INHERIT_COPY_META, phys_obj, 0, &r);

    // Release physical object refcount locally as region took ownership
    vm_object_release(phys_obj);

    if (attach_ret != 0) {
        vm_mutation_finish(space);
        return attach_ret;
    }

    // Release the semantic lock before running the distributed uRPC wait loop!
    spinlock_release(&space->lock);

    // Apply MAP locally
    int local_ret = 0;
    if (aspace->prot_domain) {
        local_ret = prot_domain_map_region(aspace->prot_domain, req->va, req->pa, req->len, req->prot);
    }

    int dist_ret = 0;
    if (local_ret == 0) {
        dist_ret = mon_vm_send_map(space, req, true);
    } else {
        dist_ret = local_ret;
    }

    // Re-acquire lock to commit or compensate
    spinlock_acquire(&space->lock);

    if (dist_ret == MON_VM_STATUS_SUCCESS) {
        // Success: commit metadata generation and synchronize active masks
        space->generation++;
        if (space->aspace) {
            space->active_cores = aspace_get_active_mask(space->aspace);
        }
        vm_mutation_finish(space); // clears mutation kind and releases lock
        return 0;
    } else {
        // Partial failure: compensate successful remote/local mappings
        if (aspace->prot_domain) {
            prot_domain_unmap_region(aspace->prot_domain, req->va, req->len);
        }

        // Send compensating UNMAP
        mon_vm_send_unmap(space, req->va, req->len, true);

        // Rollback canonical staging region
        aspace_region_detach(aspace, req->va);

        vm_mutation_finish(space);
        return dist_ret;
    }
}

int vm_unmap(vm_space_t *space, uintptr_t va, size_t len) {
    if (!space) return -1;

    int begin_ret = vm_mutation_begin(space, VM_MUTATION_UNMAP);
    if (begin_ret != 0) return begin_ret;

    address_space_t *aspace = space->aspace;
    vm_region_t *r = aspace_lookup_region(aspace, va);
    if (!r) {
        vm_mutation_finish(space);
        return -1;
    }

    // Snapshot target details before detaching
    uintptr_t snapshot_va = r->base;
    size_t snapshot_len = r->length;

    // Release semantic lock before wait
    spinlock_release(&space->lock);

    // Apply local unmap first
    if (aspace->prot_domain) {
        prot_domain_unmap_region(aspace->prot_domain, snapshot_va, snapshot_len);
    }

    // Perform distributed unmap
    int dist_ret = mon_vm_send_unmap(space, snapshot_va, snapshot_len, true);

    spinlock_acquire(&space->lock);

    if (dist_ret == MON_VM_STATUS_SUCCESS) {
        // Success: detach region and update generation
        aspace_region_detach(aspace, snapshot_va);
        space->generation++;
        if (space->aspace) {
            space->active_cores = aspace_get_active_mask(space->aspace);
        }
        vm_mutation_finish(space);
        return 0;
    } else {
        // Security-sensitive: unresolved timeout/failure on unmap poisons address space
        if (space->aspace) {
            aspace_mark_poisoned(space->aspace);
        }
        vm_mutation_finish(space);
        return dist_ret;
    }
}

int vm_protect(vm_space_t *space, uintptr_t va, size_t len, uint64_t prot, uint64_t mem_type) {
    if (!space) return -1;

    int begin_ret = vm_mutation_begin(space, VM_MUTATION_PROTECT);
    if (begin_ret != 0) return begin_ret;

    address_space_t *aspace = space->aspace;
    vm_region_t *r = aspace_lookup_region(aspace, va);
    if (!r) {
        vm_mutation_finish(space);
        return -1;
    }

    uint64_t old_prot = r->prot;

    // Release lock before distributed wait
    spinlock_release(&space->lock);

    // Apply local protection change
    if (aspace->prot_domain) {
        prot_domain_protect_region(aspace->prot_domain, va, len, prot);
    }

    // Perform distributed protect
    int dist_ret = mon_vm_send_protect(space, va, len, prot, mem_type, true);

    spinlock_acquire(&space->lock);

    if (dist_ret == MON_VM_STATUS_SUCCESS) {
        // Success: update region's prot, generation
        r->prot = prot;
        space->generation++;
        if (space->aspace) {
            space->active_cores = aspace_get_active_mask(space->aspace);
        }
        vm_mutation_finish(space);
        return 0;
    } else {
        // Failure:
        // For downgrades: unresolved failure poisons space because a core retains stronger permissions
        // For upgrades: restore old protection
        bool is_downgrade = (old_prot & ~prot) != 0;
        if (is_downgrade) {
            if (space->aspace) {
                aspace_mark_poisoned(space->aspace);
            }
        } else {
            // Restore old protection locally
            if (aspace->prot_domain) {
                prot_domain_protect_region(aspace->prot_domain, va, len, old_prot);
            }
            // Retain old protection in region metadata
            r->prot = old_prot;
        }
        vm_mutation_finish(space);
        return dist_ret;
    }
}

// Bounded distributed SPACE_DESTROY transaction
int vm_space_destroy_distributed(vm_space_t *space) {
    if (!space || !space->aspace) return -1;

    // Acquire lock and verify mutation gate is not occupied
    spinlock_acquire(&space->lock);

    // If another mutation is active, return K_ERR_BUSY. Do not wait under lock.
    if (space->mutation_kind != VM_MUTATION_NONE) {
        spinlock_release(&space->lock);
        return MON_VM_STATUS_BUSY;
    }

    // Set mutation kind to DESTROY to prevent any future mutations/rebuilds
    space->mutation_kind = VM_MUTATION_DESTROY;

    // Transition the canonical aspace to DYING
    space->aspace->state = ASPACE_STATE_DYING;

    // Snapshot target mask of realized cores
    uint64_t target_cores = space->realized_cores & ~(1ULL << hal_cpu_get_id());

    spinlock_release(&space->lock);

    // If there are remote realizations, coordinate destruction transactionally
    int wait_res = MON_VM_STATUS_SUCCESS;
    if (target_cores != 0) {
        uint32_t my_core = hal_cpu_get_id();
        mon_vm_core_state_t *state = &g_mon_vm_core_states[my_core];

        mon_vm_tx_handle_t handle;
        // Allocate pending txn manually
        spin_lock(&state->lock);
        for (int i = 0; i < MAX_PENDING_TXNS; i++) {
            if (!state->pending_txns[i].in_use) {
                state->pending_txns[i].in_use = true;
                state->pending_txns[i].handle.origin_core = my_core;
                state->pending_txns[i].handle.slot = i;
                state->pending_txns[i].handle.generation = state->next_txn_generation++;
                state->pending_txns[i].target_mask = target_cores;
                state->pending_txns[i].ack_mask = 0;
                state->pending_txns[i].success_mask = 0;
                state->pending_txns[i].nack_mask = 0;
                state->pending_txns[i].attempt_count = 0;
                state->pending_txns[i].deadline_ticks = 0;
                state->pending_txns[i].final_status = MON_VM_STATUS_SUCCESS;
                state->pending_txns[i].phase = MON_VM_TX_PHASE_PREPARED;

                handle = state->pending_txns[i].handle;
                break;
            }
        }
        spin_unlock(&state->lock);

        mon_vm_pending_txn_t *txn = &state->pending_txns[handle.slot];

        mon_vm_msg_t msg = {0};
        msg.h.version_major = MON_VM_PROTO_VERSION_MAJOR;
        msg.h.version_minor = MON_VM_PROTO_VERSION_MINOR;
        msg.h.type = MON_VM_SPACE_DESTROY;
        msg.h.flags = MON_VM_F_STRICT_ACK;
        msg.h.space_id = space->space_id;
        msg.h.expected_generation = space->generation;
        msg.h.proposed_generation = space->generation;
        msg.h.payload_len = 0;

        int send_res = mon_vm_send_txn_async(txn, &msg);
        if (send_res == MON_VM_STATUS_SUCCESS) {
            wait_res = mon_vm_wait_for_acks(handle, &msg);
        } else {
            wait_res = send_res;
        }

        spin_lock(&state->lock);
        txn->in_use = false;
        spin_unlock(&state->lock);
    }

    if (wait_res == MON_VM_STATUS_SUCCESS) {
        // Remote realizations succeeded: free local resources safely
        return vm_space_destroy(space);
    } else {
        // Failure: keep the space quarantined, mark as POISONED, clear destroy kind and lock
        spinlock_acquire(&space->lock);
        space->aspace->state = ASPACE_STATE_POISONED;
        space->mutation_kind = VM_MUTATION_NONE;
        spinlock_release(&space->lock);
        return wait_res;
    }
}
