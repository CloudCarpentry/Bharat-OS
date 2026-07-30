#include "capability.h"
#include "cap_policy.h"
#include <bharat/cpu_local.h>
#include "kernel_safety.h"
#include "bharat/urpc.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
#include "slab.h"

extern void kernel_panic(const char *message);

// @cite seL4: Formal Verification of an OS Kernel (Klein et al., 2009)
// seL4 capability model and verification-oriented discipline
#include <stddef.h>

#define MAX_CAP_TABLES ((size_t)MAX_CPUS)

static uint8_t g_cap_tables_used[MAX_CAP_TABLES];

static inline void cap_lock_two_tables(capability_table_t* a, capability_table_t* b) {
    if (a == b) {
        spin_lock(&a->lock);
        return;
    }

    if ((a->numa_node < b->numa_node) ||
        (a->numa_node == b->numa_node && a < b)) {
        spin_lock(&a->lock);
        spin_lock(&b->lock);
    } else {
        spin_lock(&b->lock);
        spin_lock(&a->lock);
    }
}

static inline void cap_unlock_two_tables(capability_table_t* a, capability_table_t* b) {
    if (a == b) {
        spin_unlock(&a->lock);
        return;
    }
    spin_unlock(&a->lock);
    spin_unlock(&b->lock);
}


/*@
  assigns \nothing;
  ensures \result == 0 || \result == 1;
*/
static int cap_rights_valid(cap_type_t type, uint64_t rights) {
    return cap_transfer_rights_valid(type, rights);
}

/*@
  requires table != \null;
  requires \valid(table);
  assigns \nothing;
  ensures \result == \null || \valid(\result);
*/
capability_entry_t* cap_find_entry(capability_table_t* table, uint32_t cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return NULL;
    }

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t* e = &table->entries[i];
        if (e->in_use != 0U && e->id == cap_id) {
            return e;
        }
    }
    return NULL;
}

#include <bharat/cap/cap_validate.h>

extern bharat_cap_status_t kernel_cap_authority_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result);

capability_table_t* cap_table_create(void) {
    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(g_cpu_locals);
      loop assigns i, g_cap_tables_used[0..MAX_CAP_TABLES-1], g_cpu_locals[0..MAX_CAP_TABLES-1].cap_table;
      loop variant BHARAT_ARRAY_SIZE(g_cpu_locals) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_cpu_locals); ++i) {
        if (g_cap_tables_used[i] == 0U) {
            g_cap_tables_used[i] = 1U;
            capability_table_t* t = &g_cpu_locals[i].cap_table;
            spin_lock_init(&t->lock);
            bh_id_allocator_init(&t->id_allocator, t->id_bitmap, BHARAT_ARRAY_SIZE(t->entries));
            t->numa_node = 0U; // Placeholder, would be set to actual node in full implementation
            t->owner_pid = 0U;
            /*@
              loop invariant 0 <= j <= BHARAT_ARRAY_SIZE(t->entries);
              loop assigns j, t->entries[0..BHARAT_ARRAY_SIZE(t->entries)-1].in_use;
              loop variant BHARAT_ARRAY_SIZE(t->entries) - j;
            */
            for (size_t j = 0; j < BHARAT_ARRAY_SIZE(t->entries); ++j) {
                t->entries[j].in_use = 0U;
                t->entries[j].parent.table = NULL;
                t->entries[j].parent.slot = UINT32_MAX;
                t->entries[j].parent.generation = 0;
                t->entries[j].first_child.table = NULL;
                t->entries[j].first_child.slot = UINT32_MAX;
                t->entries[j].first_child.generation = 0;
                t->entries[j].next_sibling.table = NULL;
                t->entries[j].next_sibling.slot = UINT32_MAX;
                t->entries[j].next_sibling.generation = 0;
                t->entries[j].generation = 0;
            }
            bharat_cap_register_authority_resolver(kernel_cap_authority_resolver);
            return t;
        }
    }

    return NULL;
}

int cap_table_init_for_process(bh_process_t* proc) {
    if (!BHARAT_PTR_NON_NULL(proc)) {
        return -1;
    }

    capability_table_t* t = cap_table_create();
    if (!t) {
        return -2;
    }

    proc->security_sandbox_ctx = t;
    return 0;
}

void cap_table_destroy(capability_table_t* table) {
    if (!table) return;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_cpu_locals); ++i) {
        if (&g_cpu_locals[i].cap_table == table) {
            g_cap_tables_used[i] = 0U;
            break;
        }
    }
}

int cap_table_grant(capability_table_t* table,
                    cap_type_t type,
                    uint64_t object_ref,
                    uint64_t rights,
                    uint32_t* out_cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || type == CAP_TYPE_NONE) {
        return -1;
    }

    if (!cap_rights_valid(type, rights) || rights == CAP_RIGHT_NONE) {
        return -3;
    }

    uint32_t found_id = 0;
    int ret = -2;

    spin_lock(&table->lock);

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i, table->entries[0..BHARAT_ARRAY_SIZE(table->entries)-1], table->next_id, found_id, ret;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    uint32_t slot_idx;
    if (bh_id_allocator_alloc(&table->id_allocator, &slot_idx) == K_OK) {
            capability_entry_t* e = &table->entries[slot_idx];
            e->id = slot_idx + 1; // Keep 1-based IDs if expected, or just use slot_idx
            e->state = CAP_STATE_LIVE;
            e->type = type;
            e->rights = rights;
            e->object_ref = object_ref;
            e->parent.table = NULL;
            e->parent.slot = UINT32_MAX;
            e->parent.generation = 0;
            e->first_child.table = NULL;
            e->first_child.slot = UINT32_MAX;
            e->first_child.generation = 0;
            e->next_sibling.table = NULL;
            e->next_sibling.slot = UINT32_MAX;
            e->next_sibling.generation = 0;
            e->generation++;

            // Default owner core to the core creating the capability
            e->owner_core = hal_cpu_get_id();

            e->instance_id.origin_core = hal_cpu_get_id();
            e->instance_id.object_id = e->object_ref;
            e->instance_id.slot_gen = e->generation;
            e->instance_id.rights_digest = e->rights;
            e->revocation_epoch = 0;

            e->in_use = 1U;
            found_id = e->id | (e->generation << 16);
            ret = 0;
    }

    spin_unlock(&table->lock);

    if (ret == 0 && out_cap_id) {
        *out_cap_id = found_id;
    }

    return ret;
}

int cap_table_lookup(const capability_table_t* table,
                     uint32_t cap_id,
                     cap_type_t required_type,
                     uint64_t required_rights,
                     capability_entry_t* out_entry) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return -1;
    }

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    // If generation is 0, we treat it as un-versioned for backwards compatibility
    // if tests use raw IDs, else we strictly enforce.

    capability_entry_t found_entry = {0};
    int ret = -4;

    // table must be cast to non-const for spinlock.
    // In a real implementation we might use a reader-writer lock or similar.
    capability_table_t* t = (capability_table_t*)table;
    spin_lock(&t->lock);

    /*@
      loop invariant 0 <= i <= BHARAT_ARRAY_SIZE(table->entries);
      loop assigns i, found_entry, ret;
      loop variant BHARAT_ARRAY_SIZE(table->entries) - i;
    */
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        const capability_entry_t* e = &table->entries[i];
        if (e->in_use != 0U && e->id == id_only) {
            if (generation != 0 && e->generation != generation) {
                ret = -6; // Stale handle
                break;
            }
            if (e->state != CAP_STATE_LIVE) {
                ret = -7; // Not live
                break;
            }
            if (required_type != CAP_TYPE_NONE && e->type != required_type) {
                ret = -2;
                break;
            }
            if ((e->rights & required_rights) != required_rights) {
                ret = -3;
                break;
            }
            found_entry = *e;
            ret = 0;
            break;
        }
    }

    spin_unlock(&t->lock);

    if (ret == 0 && out_entry) {
        *out_entry = found_entry;
    }

    return ret;
}

// Helper to implement local/same-core delegation without cross-core locks
static int cap_table_delegate_local(capability_table_t* src,
                                    capability_table_t* dst,
                                    uint32_t cap_id,
                                    uint64_t delegated_rights,
                                    uint32_t* out_new_cap_id) {
    cap_lock_two_tables(src, dst);

    int ret = -2;
    uint32_t src_slot_idx = UINT32_MAX;
    capability_entry_t* src_entry = NULL;

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(src->entries); ++i) {
        if (src->entries[i].in_use != 0U && src->entries[i].id == id_only) {
            if (generation != 0 && src->entries[i].generation != generation) {
                break; // Stale handle
            }
            if (src->entries[i].state != CAP_STATE_LIVE) {
                break; // Not live
            }
            src_entry = &src->entries[i];
            src_slot_idx = (uint32_t)i;
            break;
        }
    }

    if (src_entry &&
        ((src_entry->rights & CAP_RIGHT_DELEGATE) != 0U) &&
        ((src_entry->rights & delegated_rights) == delegated_rights) &&
        cap_rights_valid(src_entry->type, delegated_rights) &&
        (delegated_rights != CAP_RIGHT_NONE)) {

        uint32_t found_id = 0;
        ret = -2;

        uint32_t dst_slot_idx;
        if (bh_id_allocator_alloc(&dst->id_allocator, &dst_slot_idx) == K_OK) {
                capability_entry_t* dst_entry = &dst->entries[dst_slot_idx];
                dst_entry->id = dst_slot_idx + 1;
                dst_entry->state = CAP_STATE_LIVE;
                dst_entry->type = src_entry->type;
                dst_entry->rights = delegated_rights;
                dst_entry->object_ref = src_entry->object_ref;
                dst_entry->flags = src_entry->flags;

                dst_entry->parent.table = src;
                dst_entry->parent.slot = src_slot_idx;
                dst_entry->parent.generation = src_entry->generation;

                dst_entry->first_child.table = NULL;
                dst_entry->first_child.slot = UINT32_MAX;
                dst_entry->first_child.generation = 0;

                dst_entry->next_sibling = src_entry->first_child;

                dst_entry->generation++;
                dst_entry->owner_core = src_entry->owner_core; // Delegate retains owner core unless explicity transferred

                // Inherit distributed instance ID
                dst_entry->instance_id = src_entry->instance_id;
                dst_entry->instance_id.rights_digest = delegated_rights;
                dst_entry->instance_id.slot_gen = dst_entry->generation;
                dst_entry->revocation_epoch = src_entry->revocation_epoch;

                dst_entry->in_use = 1U;

                src_entry->first_child.table = dst;
                src_entry->first_child.slot = dst_slot_idx;
                src_entry->first_child.generation = dst_entry->generation;

                found_id = dst_entry->id | (dst_entry->generation << 16);
                ret = 0;
        }

        if (ret == 0 && out_new_cap_id) {
            *out_new_cap_id = found_id;
        }
    } else {
        ret = -5;
    }

    cap_unlock_two_tables(src, dst);

    return ret;
}

// Global structure for passing delegation arguments across cores via uRPC.
// Since uRPC payloads are 56 bits, we use a global array indexed by the
// source core to pass the delegation parameters safely without passing
// stack pointers across cores.
typedef struct {
    capability_table_t* src;
    capability_table_t* dst;
    uint32_t src_slot_idx;
    uint32_t src_generation;
    cap_type_t type;
    uint64_t rights;
    uint64_t object_ref;
    uint32_t flags;
    uint32_t owner_core;
    cap_instance_id_t instance_id;
    uint64_t revocation_epoch;
    cap_handle_t src_first_child;  // Metadata for linking sibling list
    volatile int status;           // Output from dest
    volatile uint32_t new_cap_id;  // Output from dest
    volatile uint32_t dst_slot;    // Output from dest
    volatile uint32_t dst_generation; // Output from dest
    volatile bool ack_received;
} cap_delegate_req_t;

static cap_delegate_req_t g_cap_delegations[MAX_CPUS];

// Counter for synchronous capability revokes
volatile int g_revoke_acks_needed[MAX_CPUS];

int cap_table_delegate(capability_table_t* src,
                       capability_table_t* dst,
                       uint32_t cap_id,
                       uint64_t delegated_rights,
                       uint32_t* out_new_cap_id) {
    if (!BHARAT_PTR_NON_NULL(src) || !BHARAT_PTR_NON_NULL(dst) || cap_id == 0U) {
        return -1;
    }

    uint32_t current_core = hal_cpu_get_id();
    uint32_t target_core = MAX_CPUS;
    bool is_local = true;

    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (&g_cpu_locals[i].cap_table == dst) {
            target_core = i;
            if (i != current_core) {
                is_local = false;
            }
            break;
        }
    }

    if (is_local || target_core == MAX_CPUS || urpc_channel_get_state(target_core) != URPC_CHANNEL_BOUND) {
        return cap_table_delegate_local(src, dst, cap_id, delegated_rights, out_new_cap_id);
    }

    // --- CROSS-CORE DELEGATION via uRPC ---

    spin_lock(&src->lock);

    int ret = -2;
    uint32_t src_slot_idx = UINT32_MAX;
    capability_entry_t* src_entry = NULL;

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(src->entries); ++i) {
        if (src->entries[i].in_use != 0U && src->entries[i].id == id_only) {
            if (generation != 0 && src->entries[i].generation != generation) {
                break; // Stale handle
            }
            if (src->entries[i].state != CAP_STATE_LIVE) {
                break; // Not live
            }
            src_entry = &src->entries[i];
            src_slot_idx = (uint32_t)i;
            break;
        }
    }

    if (!src_entry ||
        ((src_entry->rights & CAP_RIGHT_DELEGATE) == 0U) ||
        ((src_entry->rights & delegated_rights) != delegated_rights) ||
        !cap_rights_valid(src_entry->type, delegated_rights) ||
        (delegated_rights == CAP_RIGHT_NONE)) {

        spin_unlock(&src->lock);
        return -5;
    }

    cap_delegate_req_t* req = &g_cap_delegations[current_core];
    req->src = src;
    req->dst = dst;
    req->src_slot_idx = src_slot_idx;
    req->src_generation = src_entry->generation;
    req->type = src_entry->type;
    req->rights = delegated_rights;
    req->object_ref = src_entry->object_ref;
    req->flags = src_entry->flags;
    req->owner_core = src_entry->owner_core;
    req->instance_id = src_entry->instance_id;
    req->instance_id.rights_digest = delegated_rights;
    req->revocation_epoch = src_entry->revocation_epoch;
    req->src_first_child = src_entry->first_child;
    req->status = -1;
    req->new_cap_id = 0;
    req->dst_slot = UINT32_MAX;
    req->dst_generation = 0;
    req->ack_received = false;

    spin_unlock(&src->lock);

    uint64_t payload = current_core;

    if (urpc_channel_get_state(target_core) != URPC_CHANNEL_BOUND) {
        return -6;
    }

    urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_DELEGATE_REQ, payload));

    // Bounded transactional wait
    uint64_t start_ticks = hal_timer_monotonic_ticks();
    uint64_t timeout_ticks = 10;
    bool timed_out = false;

    while (!req->ack_received) {
        if (hal_timer_monotonic_ticks() - start_ticks > timeout_ticks) {
            timed_out = true;
            break;
        }
        extern void arch_cpu_relax(void);
        arch_cpu_relax();
        extern void vmm_process_urpc_messages(void);
        vmm_process_urpc_messages();
    }

    if (timed_out) {
        return K_ERR_TIMEOUT;
    }

    if (req->status != 0) {
        return req->status;
    }

    spin_lock(&src->lock);

    src_entry = &src->entries[src_slot_idx];
    if (src_entry->in_use != 0U && src_entry->id == id_only && src_entry->generation == req->src_generation && src_entry->state == CAP_STATE_LIVE) {
        src_entry->first_child.table = dst;
        src_entry->first_child.slot = req->dst_slot;
        src_entry->first_child.generation = req->dst_generation;

        if (out_new_cap_id) {
            *out_new_cap_id = req->new_cap_id | (req->dst_generation << 16);
        }
        ret = 0;
    } else {
        // Transactional rollback
        ret = -7;
    }

    spin_unlock(&src->lock);

    if (ret != 0 && req->status == 0) {
        // Rollback target capability with the matching lineage-aware payload
        uint64_t rollback_payload = ((uint64_t)current_core << 48) |
                                   ((uint64_t)req->src_slot_idx << 40) |
                                   ((uint64_t)req->src_generation << 24) |
                                   current_core;
        urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_REVOKE, rollback_payload));
    }

    return ret;
}

void cap_handle_delegate_req(uint64_t payload, uint32_t source_core) {
    uint32_t req_core = (uint32_t)payload;
    if (req_core >= MAX_CPUS) return;

    cap_delegate_req_t req_clone = g_cap_delegations[req_core];

    capability_table_t* dst = req_clone.dst;
    if (!dst) {
        uint64_t ack_payload = ((uint64_t)-1 << 32) | req_core;
        urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_DELEGATE_ACK, ack_payload));
        return;
    }

    uint32_t current_core = hal_cpu_get_id();
    bool table_belongs_to_me = false;
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (&g_cpu_locals[i].cap_table == dst) {
            if (i == current_core) {
                table_belongs_to_me = true;
            }
            break;
        }
    }

    if (!table_belongs_to_me) {
        uint64_t ack_payload = ((uint64_t)-2 << 32) | req_core;
        urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_DELEGATE_ACK, ack_payload));
        return;
    }

    spin_lock(&dst->lock);

    int ret = -2;
    uint32_t found_id = 0;
    uint32_t dst_slot = UINT32_MAX;
    uint32_t dst_generation = 0;

    uint32_t dst_slot_idx;
    if (bh_id_allocator_alloc(&dst->id_allocator, &dst_slot_idx) == K_OK) {
        capability_entry_t* dst_entry = &dst->entries[dst_slot_idx];
        dst_entry->id = dst_slot_idx + 1;
        dst_entry->state = CAP_STATE_LIVE;
        dst_entry->type = req_clone.type;
        dst_entry->rights = req_clone.rights;
        dst_entry->object_ref = req_clone.object_ref;
        dst_entry->flags = req_clone.flags;
        dst_entry->owner_core = req_clone.owner_core;

        dst_entry->instance_id = req_clone.instance_id;
        dst_entry->revocation_epoch = req_clone.revocation_epoch;

        dst_entry->parent.table = req_clone.src;
        dst_entry->parent.slot = req_clone.src_slot_idx;
        dst_entry->parent.generation = req_clone.src_generation;

        dst_entry->first_child.table = NULL;
        dst_entry->first_child.slot = UINT32_MAX;
        dst_entry->first_child.generation = 0;

        dst_entry->next_sibling = req_clone.src_first_child;

        dst_entry->generation++;
        dst_entry->instance_id.slot_gen = dst_entry->generation;
        dst_entry->in_use = 1U;

        found_id = dst_entry->id;
        dst_slot = dst_slot_idx;
        dst_generation = dst_entry->generation;
        ret = 0;
    }

    spin_unlock(&dst->lock);

    uint64_t ack_payload = ((uint64_t)(uint32_t)ret << 32) |
                           ((uint64_t)dst_slot << 24) |
                           ((uint64_t)dst_generation << 8) |
                           ((uint64_t)found_id << 40) |
                           req_core;

    urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_DELEGATE_ACK, ack_payload));
}

void cap_handle_delegate_ack(uint64_t payload) {
    uint32_t req_core = (uint32_t)(payload & 0xFF);
    if (req_core >= MAX_CPUS) return;

    int32_t status = (int32_t)(int8_t)((payload >> 32) & 0xFF);
    uint32_t dst_slot = (uint32_t)((payload >> 24) & 0xFF);
    uint32_t dst_generation = (uint32_t)((payload >> 8) & 0xFFFF);
    uint32_t new_cap_id = (uint32_t)((payload >> 40) & 0xFFFF);

    cap_delegate_req_t* req = &g_cap_delegations[req_core];
    req->status = status;
    if (status == 0) {
        req->dst_slot = dst_slot;
        req->dst_generation = dst_generation;
        req->new_cap_id = new_cap_id;
    }
    req->ack_received = true;
}

void cap_handle_revoke_req(uint64_t payload, uint32_t source_core) {
    uint32_t origin_cpu = (uint32_t)((payload >> 48) & 0xFF);
    uint32_t src_slot = (uint32_t)((payload >> 40) & 0xFF);
    uint32_t src_generation = (uint32_t)((payload >> 24) & 0xFFFF);
    uint32_t req_core = (uint32_t)(payload & 0xFF);

    uint32_t current_core = hal_cpu_get_id();
    capability_table_t* table = &g_cpu_locals[current_core].cap_table;

    // Scan our local capability table for descendants of the specified parent lineage
    spin_lock(&table->lock);
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t *entry = &table->entries[i];
        if (entry->in_use != 0U && entry->state == CAP_STATE_LIVE) {
            bool is_descendant = false;
            if (entry->parent.table != NULL) {
                uint32_t p_core = MAX_CPUS;
                for (uint32_t j = 0; j < MAX_CPUS; j++) {
                    if (&g_cpu_locals[j].cap_table == entry->parent.table) {
                        p_core = j;
                        break;
                    }
                }
                if (p_core == origin_cpu && entry->parent.slot == src_slot && entry->parent.generation == src_generation) {
                    is_descendant = true;
                }
            }

            if (is_descendant) {
                entry->rights = 0U;
                entry->flags = 0U;
                entry->object_ref = 0U;

                entry->parent.table = NULL;
                entry->parent.slot = UINT32_MAX;
                entry->parent.generation = 0;

                entry->first_child.table = NULL;
                entry->first_child.slot = UINT32_MAX;
                entry->first_child.generation = 0;

                entry->next_sibling.table = NULL;
                entry->next_sibling.slot = UINT32_MAX;
                entry->next_sibling.generation = 0;

                entry->generation++;
                entry->state = CAP_STATE_FREE;
                entry->in_use = 0U;
                bh_id_allocator_free(&table->id_allocator, (uint32_t)i);
            }
        }
    }
    spin_unlock(&table->lock);

    urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_REVOKE_ACK, req_core));
}

void cap_handle_revoke_ack(uint64_t payload) {
    uint32_t req_core = (uint32_t)payload;
    if (req_core < MAX_CPUS) {
        g_revoke_acks_needed[req_core]--;
    }
}

#ifdef BHARAT_CONFIG_CAP_REVOKE_MAX
#define CAP_REVOKE_MAX BHARAT_CONFIG_CAP_REVOKE_MAX
#else
#define CAP_REVOKE_MAX 64
#endif

// Helper to lock multiple tables in total order to prevent ABBA deadlocks.
// We only support up to 4 tables at once (e.g. parent, table, prev, sibling).
void cap_lock_tables_sorted(capability_table_t** tables, size_t count) {
    // Simple insertion sort by NUMA then memory address
    for (size_t i = 1; i < count; i++) {
        capability_table_t* key = tables[i];
        int j = i - 1;
        while (j >= 0 && ((tables[j]->numa_node > key->numa_node) ||
                          (tables[j]->numa_node == key->numa_node && tables[j] > key))) {
            tables[j + 1] = tables[j];
            j = j - 1;
        }
        tables[j + 1] = key;
    }

    // Lock uniquely
    capability_table_t* last_locked = NULL;
    for (size_t i = 0; i < count; i++) {
        if (tables[i] != last_locked) {
            spin_lock(&tables[i]->lock);
            last_locked = tables[i];
        }
    }
}

void cap_unlock_tables_sorted(capability_table_t** tables, size_t count) {
    // Unlock uniquely in reverse order
    capability_table_t* last_unlocked = NULL;
    for (int i = (int)count - 1; i >= 0; i--) {
        if (tables[i] != last_unlocked) {
            spin_unlock(&tables[i]->lock);
            last_unlocked = tables[i];
        }
    }
}

static bool cap_table_pointer_is_known(const capability_table_t* table) {
    if (table == NULL) {
        return false;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_cpu_locals); ++i) {
        if (&g_cpu_locals[i].cap_table == table) {
            return true;
        }
    }

    return false;
}

int cap_table_revoke(capability_table_t* table, uint32_t cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return -1;
    }

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    // Iterative tree walk to revoke children safely.
    // Use a fixed-size stack to avoid kmalloc in core kernel path.
    // 64 entries = 1KB on stack, which is safe for kernel stacks.
    cap_handle_t stack[64];
    size_t sp = 0;

    spin_lock(&table->lock);

    uint32_t root_slot = UINT32_MAX;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        if (table->entries[i].in_use != 0U && table->entries[i].id == id_only) {
            if (generation != 0 && table->entries[i].generation != generation) {
                break; // Stale handle
            }
            if (table->entries[i].state == CAP_STATE_FREE) {
                break; // Already free
            }
            root_slot = (uint32_t)i;
            table->entries[i].state = CAP_STATE_REVOKING;
            break;
        }
    }

    if (root_slot == UINT32_MAX) {
        spin_unlock(&table->lock);
        return -2;
    }

    if (root_slot >= BHARAT_ARRAY_SIZE(table->entries)) {
        spin_unlock(&table->lock);
        return -2;
    }

    capability_entry_t* root_entry = &table->entries[root_slot];
    uint32_t root_gen = root_entry->generation;

    capability_table_t* parent_table = root_entry->parent.table;
    uint32_t parent_slot = root_entry->parent.slot;
    uint32_t parent_gen = root_entry->parent.generation;

    spin_unlock(&table->lock);

    if (parent_table) {
        // Collect all tables required for the sibling chain to lock them correctly and prevent deadlocks
        capability_table_t* tables_to_lock[16];
        size_t num_tables = 0;

        tables_to_lock[num_tables++] = table;
        tables_to_lock[num_tables++] = parent_table;

        // Resolve lock inversion safely using pre-sorted multi-lock
        cap_lock_tables_sorted(tables_to_lock, num_tables);

        // Verify root hasn't been reallocated
        if (root_entry->in_use != 0U && root_entry->generation == root_gen) {
            // Verify parent hasn't been reallocated
            if (parent_slot >= BHARAT_ARRAY_SIZE(parent_table->entries)) {
                cap_unlock_tables_sorted(tables_to_lock, num_tables);
                return -2;
            }
            capability_entry_t* parent = &parent_table->entries[parent_slot];
            if (parent->in_use != 0U && parent->generation == parent_gen) {
                cap_handle_t sibling = parent->first_child;
                cap_handle_t prev = { .table = NULL, .slot = UINT32_MAX, .generation = 0 };

                while (sibling.table != NULL && sibling.slot != UINT32_MAX) {
                    if (sibling.table != table && sibling.table != parent_table) {
                        break;
                    }
                    // Sanity check to avoid bounds violation
                    if (sibling.slot >= BHARAT_ARRAY_SIZE(sibling.table->entries)) {
                        break;
                    }
                    if (sibling.table == table && sibling.slot == root_slot && sibling.generation == root_gen) {
                        if (prev.table != NULL && prev.slot != UINT32_MAX) {
                            if (prev.table == table) {
                                table->entries[prev.slot].next_sibling = root_entry->next_sibling;
                            } else if (prev.table == parent_table) {
                                parent_table->entries[prev.slot].next_sibling = root_entry->next_sibling;
                            } else {
                                // If the previous sibling is in an unlocked table, we conservatively abort
                                // this specific unlink to avoid dynamic lock inversion (a background cleanup task
                                // or global epoch grace period handles unreachable garbage in a full design).
                                // For now, we assume sibling resides in table or parent_table.
                            }
                        } else {
                            parent->first_child = root_entry->next_sibling;
                        }
                        break;
                    }

                    prev = sibling;
                    // Traverse down the sibling chain. We only safely follow links inside the tables we locked.
                    if (sibling.table == table) {
                        sibling = table->entries[sibling.slot].next_sibling;
                    } else if (sibling.table == parent_table) {
                        sibling = parent_table->entries[sibling.slot].next_sibling;
                    } else {
                        break; // Stop traversal to avoid dynamic lock inversion
                    }
                }
            }
        }

        cap_unlock_tables_sorted(tables_to_lock, num_tables);
    }

    // Increment revocation epoch BEFORE broadcast
    spin_lock(&table->lock);
    uint64_t epoch = 0;
    if (root_entry->in_use != 0U && root_entry->generation == root_gen) {
        root_entry->revocation_epoch++;
        epoch = root_entry->revocation_epoch;
    }
    spin_unlock(&table->lock);

    // Iterative tree walk to revoke children safely.
    stack[sp].table = table;
    stack[sp].slot = root_slot;
    stack[sp].generation = root_gen;
    sp++;

    uint32_t current_core = hal_cpu_get_id();

    // We check g_pmm_initialized as a proxy for early boot since PMM initializes URPC safely.
    // Use weak linkage or simple dummy for tests.
    __attribute__((weak)) extern bool g_pmm_initialized;
    bool pmm_is_initialized = &g_pmm_initialized ? g_pmm_initialized : true;

    // In Bharat-OS capability revoke operations must degrade safely to local-only behavior
    // until SMP and URPC distributed infrastructure are fully initialized.
    if (current_core < MAX_CPUS && pmm_is_initialized) {
        // Broadcast revocation to other cores via URPC only if we are on a valid, initialized core
        g_revoke_acks_needed[current_core] = 0;
        for (uint32_t c = 0; c < MAX_CPUS; c++) {
            if (c != current_core && urpc_channel_get_state(c) == URPC_CHANNEL_BOUND) {
                // Encode the lineage into the payload so the target can match parent precisely
                uint64_t payload = ((uint64_t)current_core << 48) |
                                   ((uint64_t)root_slot << 40) |
                                   ((uint64_t)root_gen << 24) |
                                   ((uint64_t)(epoch & 0xFFFF) << 8) |
                                   current_core;
                urpc_bootstrap_send(c, urpc_pack_msg(URPC_CAP_REVOKE, payload));
                g_revoke_acks_needed[current_core]++;
            }
        }

        // Bounded transactional wait with explicit timeout / panic policy
        uint64_t start_ticks = hal_timer_monotonic_ticks();
        uint64_t timeout_ticks = 10; // 10ms
        bool timed_out = false;

        while (g_revoke_acks_needed[current_core] > 0) {
            if (hal_timer_monotonic_ticks() - start_ticks > timeout_ticks) {
                timed_out = true;
                break;
            }
            extern void arch_cpu_relax(void);
            arch_cpu_relax();
            extern void vmm_process_urpc_messages(void);
            vmm_process_urpc_messages(); // check if acks arrived and process global messages
        }

        if (timed_out) {
            kernel_panic("Capability Revocation Timeout: Bounded synchronization failed! Halted to prevent security breach.");
        }
    }

    while (sp > 0) {
        cap_handle_t frame = stack[--sp];

        if (!cap_table_pointer_is_known(frame.table)) {
            continue;
        }

        spin_lock(&frame.table->lock);

        if (frame.slot >= BHARAT_ARRAY_SIZE(frame.table->entries)) {
            spin_unlock(&frame.table->lock);
            continue;
        }

        capability_entry_t* cap = &frame.table->entries[frame.slot];
        if (cap->in_use == 0U || cap->generation != frame.generation) {
            spin_unlock(&frame.table->lock);
            continue;
        }

        // Before wiping the capability, queue up its children and siblings to be processed next.
        // It's a stack (DFS traversal) so children/siblings will be processed after unlocking this frame.
        // We do siblings first so they are processed AFTER children (DFS depth first into children)

        if (frame.table != table || frame.slot != root_slot) { // Don't follow root's siblings!
            if (cap->next_sibling.table != NULL && cap->next_sibling.slot != UINT32_MAX) {
                if (sp >= 64) {
                    spin_unlock(&frame.table->lock);
                    return -3; // bounded-stack overflow
                }
                stack[sp] = cap->next_sibling;
                sp++;
            }
        }

        // Push children onto the stack
        if (cap->first_child.table != NULL && cap->first_child.slot != UINT32_MAX) {
            if (sp >= 64) {
                spin_unlock(&frame.table->lock);
                return -3; // bounded-stack overflow
            }
            stack[sp] = cap->first_child;
            sp++;
        }

        // Wipe the capability (Free pool)

        // If this capability was an endpoint, we should mark the endpoint as revoked/closed
        // if this was the original or only capability for it?
        // Let's rely on the endpoint state being verified during send/receive

        cap->rights = 0U;
        cap->flags = 0U;
        cap->object_ref = 0U;

        cap->parent.table = NULL;
        cap->parent.slot = UINT32_MAX;
        cap->parent.generation = 0;

        cap->first_child.table = NULL;
        cap->first_child.slot = UINT32_MAX;
        cap->first_child.generation = 0;

        cap->next_sibling.table = NULL;
        cap->next_sibling.slot = UINT32_MAX;
        cap->next_sibling.generation = 0;

        cap->generation++;
        cap->state = CAP_STATE_FREE;
        cap->in_use = 0U;
        bh_id_allocator_free(&frame.table->id_allocator, frame.slot);

        spin_unlock(&frame.table->lock);
    }

    return 0;
}


static kstatus_t cap_validate_rights_internal(cap_rights_mask_t entry_rights, cap_rights_mask_t required_rights) {
    if ((entry_rights & required_rights) != required_rights) {
        return K_ERR_CAP_DENIED;
    }
    return K_OK;
}

static kstatus_t cap_validate_object_type_internal(cap_type_t entry_type, cap_type_t expected_type) {
    if (expected_type != CAP_TYPE_NONE && entry_type != expected_type) {
        return K_ERR_CAP_WRONG_TYPE;
    }
    return K_OK;
}

static kstatus_t cap_validate_scope_internal(const capability_table_t *table, uint32_t requester_pid) {
    // If requester_pid is 0, we assume scope check is not requested or it's a kernel internal caller
    if (requester_pid == 0) {
        return K_OK;
    }

    if (table->owner_pid != 0 && table->owner_pid != requester_pid) {
        return K_ERR_CAP_DENIED;
    }
    return K_OK;
}

static kstatus_t cap_validate_generation_internal(uint32_t entry_gen, uint64_t expected_gen) {
    if (expected_gen != 0 && (uint32_t)expected_gen != entry_gen) {
        return K_ERR_CAP_STALE;
    }
    return K_OK;
}

kstatus_t cap_validate_ex(capability_table_t *table,
                          const cap_validation_request_t *req,
                          capability_entry_t *out_entry) {
    if (!table || !req) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t id_only = bh_cap_index(req->cap_id);
    uint32_t handle_gen = bh_cap_generation(req->cap_id);

    kstatus_t status = K_ERR_NOT_FOUND;

    spin_lock(&table->lock);

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t *e = &table->entries[i];
        if (e->in_use != 0U && e->id == id_only) {
            // 1. Generation check (handle vs entry)
            if (handle_gen != 0 && e->generation != handle_gen) {
                status = K_ERR_CAP_STALE;
                break;
            }

            // 2. Expected generation check (request vs entry)
            status = cap_validate_generation_internal(e->generation, req->expected_generation);
            if (status != K_OK) break;

            // 3. State check
            if (e->state != CAP_STATE_LIVE) {
                status = K_ERR_CAP_REVOKED;
                break;
            }

            // 4. Object type check
            status = cap_validate_object_type_internal(e->type, req->expected_object_type);
            if (status != K_OK) break;

            // 5. Rights check
            status = cap_validate_rights_internal(e->rights, req->required_rights);
            if (status != K_OK) break;

            // 6. Scope check
            status = cap_validate_scope_internal(table, req->requester_pid);
            if (status != K_OK) break;

            if (out_entry) {
                *out_entry = *e; // Copy by value!
            }
            status = K_OK;
            break;
        }
    }

    spin_unlock(&table->lock);

    if (status != K_OK) {
        extern void console_log(int level, const char* fmt, ...);
        console_log(2,
            "AUDIT [DENY]: caller_pid=%u cap_id=%u expected_type=%u required_rights=0x%llx reason=%d CPU=%u\n",
            req->requester_pid, req->cap_id, req->expected_object_type,
            (unsigned long long)req->required_rights, (int)status, hal_cpu_get_id());
    }

    return status;
}

kstatus_t cap_lookup_thread(const capability_table_t *table, uint32_t cap_id, cap_rights_mask_t required_rights, bh_thread_object_t *out) {
    if (!table || !out) return K_ERR_INVALID_ARG;
    if (!bh_cap_is_valid_encoding(cap_id)) return K_ERR_CAP_INVALID;

    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = CAP_TYPE_THREAD,
        .required_rights = required_rights,
        .requester_pid = table->owner_pid,
        .expected_generation = bh_cap_generation(cap_id)
    };

    capability_entry_t e;
    kstatus_t st = cap_validate_ex((capability_table_t *)table, &req, &e);
    if (st != K_OK) return st;

    out->tid = (uint64_t)e.object_ref;
    out->thread = (struct bh_thread *)e.object_ref;
    return K_OK;
}

kstatus_t cap_lookup_process(const capability_table_t *table, uint32_t cap_id, cap_rights_mask_t required_rights, bh_process_object_t *out) {
    if (!table || !out) return K_ERR_INVALID_ARG;
    if (!bh_cap_is_valid_encoding(cap_id)) return K_ERR_CAP_INVALID;

    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = CAP_TYPE_PROCESS,
        .required_rights = required_rights,
        .requester_pid = table->owner_pid,
        .expected_generation = bh_cap_generation(cap_id)
    };

    capability_entry_t e;
    kstatus_t st = cap_validate_ex((capability_table_t *)table, &req, &e);
    if (st != K_OK) return st;

    out->process = (struct bh_process *)e.object_ref;
    if (out->process) { out->pid = out->process->process_id; } else { out->pid = 0; }
    return K_OK;
}

kstatus_t cap_lookup_memory(const capability_table_t *table, uint32_t cap_id, cap_rights_mask_t required_rights, bh_memory_object_t *out) {
    if (!table || !out) return K_ERR_INVALID_ARG;
    if (!bh_cap_is_valid_encoding(cap_id)) return K_ERR_CAP_INVALID;

    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = CAP_TYPE_MEMORY,
        .required_rights = required_rights,
        .requester_pid = table->owner_pid,
        .expected_generation = bh_cap_generation(cap_id)
    };

    capability_entry_t e;
    kstatus_t st = cap_validate_ex((capability_table_t *)table, &req, &e);
    if (st != K_OK) return st;

    out->base = (phys_addr_t)e.object_ref;
    out->size = 4096;
    out->flags = e.flags;
    return K_OK;
}

kstatus_t cap_lookup_endpoint(const capability_table_t *table, uint32_t cap_id, cap_rights_mask_t required_rights, bh_endpoint_object_t *out) {
    if (!table || !out) return K_ERR_INVALID_ARG;
    if (!bh_cap_is_valid_encoding(cap_id)) return K_ERR_CAP_INVALID;

    cap_validation_request_t req = {
        .cap_id = cap_id,
        .expected_object_type = CAP_TYPE_ENDPOINT,
        .required_rights = required_rights,
        .requester_pid = table->owner_pid,
        .expected_generation = bh_cap_generation(cap_id)
    };

    capability_entry_t e;
    kstatus_t st = cap_validate_ex((capability_table_t *)table, &req, &e);
    if (st != K_OK) return st;

    out->endpoint_id = (uintptr_t)e.id;
    out->endpoint_ref = (void *)e.object_ref;
    return K_OK;
}

static cap_type_t map_bharat_type_to_kernel(bharat_cap_object_type_t type) {
    switch (type) {
        case BHARAT_CAP_OBJ_NONE:        return CAP_TYPE_NONE;
        case BHARAT_CAP_OBJ_SERVICE:     return CAP_TYPE_NONE;
        case BHARAT_CAP_OBJ_NET_IFACE:   return CAP_TYPE_NETDEV;
        case BHARAT_CAP_OBJ_ROUTE_TABLE: return CAP_TYPE_NONE;
        case BHARAT_CAP_OBJ_ENDPOINT:    return CAP_TYPE_ENDPOINT;
        case BHARAT_CAP_OBJ_PROCESS:     return CAP_TYPE_PROCESS;
        case BHARAT_CAP_OBJ_VM_SPACE:    return CAP_TYPE_MEMORY;
        case BHARAT_CAP_OBJ_DEVICE:      return CAP_TYPE_ACCEL_DEVICE;
        case BHARAT_CAP_OBJ_DMA_DOMAIN:  return CAP_TYPE_DMA_DOMAIN;
        default:                         return CAP_TYPE_NONE;
    }
}

static cap_rights_mask_t map_bharat_rights_to_kernel(cap_type_t type, uint64_t rights) {
    cap_rights_mask_t r = 0;
    if (type == CAP_TYPE_MEMORY || type == CAP_TYPE_NET_BUFFER) {
        if (rights & BHARAT_CAP_RIGHT_READ)    r |= CAP_RIGHT_MEMORY_MAP;
        if (rights & BHARAT_CAP_RIGHT_WRITE)   r |= CAP_RIGHT_MEMORY_UNMAP;
        if (rights & BHARAT_CAP_RIGHT_EXECUTE) r |= CAP_RIGHT_MEMORY_SHARE;
        if (rights & BHARAT_CAP_RIGHT_GRANT)   r |= CAP_RIGHT_DELEGATE;
    } else if (type == CAP_TYPE_ENDPOINT || type == CAP_TYPE_CRYPTO_ENDPOINT) {
        if (rights & BHARAT_CAP_RIGHT_READ)    r |= CAP_RIGHT_ENDPOINT_RECEIVE;
        if (rights & BHARAT_CAP_RIGHT_WRITE)   r |= CAP_RIGHT_ENDPOINT_SEND;
        if (rights & BHARAT_CAP_RIGHT_GRANT)   r |= CAP_RIGHT_DELEGATE;
    } else if (type == CAP_TYPE_PROCESS) {
        if (rights & BHARAT_CAP_RIGHT_READ)    r |= CAP_RIGHT_PROCESS_MANAGE;
        if (rights & BHARAT_CAP_RIGHT_WRITE)   r |= CAP_RIGHT_PROCESS_MANAGE;
        if (rights & BHARAT_CAP_RIGHT_EXECUTE) r |= CAP_RIGHT_RESOURCE_ALLOC;
        if (rights & BHARAT_CAP_RIGHT_GRANT)   r |= CAP_RIGHT_DELEGATE;
    } else if (type == CAP_TYPE_THREAD || type == CAP_TYPE_SCHED) {
        if (rights & BHARAT_CAP_RIGHT_EXECUTE) r |= CAP_RIGHT_SCHEDULE;
        if (rights & BHARAT_CAP_RIGHT_GRANT)   r |= CAP_RIGHT_DELEGATE;
    } else {
        if (rights & BHARAT_CAP_RIGHT_READ)    r |= CAP_RIGHT_READ;
        if (rights & BHARAT_CAP_RIGHT_WRITE)   r |= CAP_RIGHT_WRITE;
        if (rights & BHARAT_CAP_RIGHT_EXECUTE) r |= CAP_RIGHT_EXECUTE;
        if (rights & BHARAT_CAP_RIGHT_GRANT)   r |= CAP_RIGHT_DELEGATE;
    }
    return r;
}

bharat_cap_status_t kernel_cap_authority_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result)
{
    (void)required_scope;
    (void)expected_object_id;
    uint32_t cpu = hal_cpu_get_id();
    capability_table_t* table = NULL;
    if (g_cpu_locals[cpu].current && g_cpu_locals[cpu].current->process) {
        table = g_cpu_locals[cpu].current->process->security_sandbox_ctx;
    }

    if (!table) {
        if (out_result) {
            out_result->allowed = false;
            out_result->status = BHARAT_CAP_NOT_FOUND;
        }
        return BHARAT_CAP_NOT_FOUND;
    }

    cap_validation_request_t req = {0};
    req.cap_id = (uint32_t)handle;
    req.expected_object_type = map_bharat_type_to_kernel(expected_object_type);
    req.required_rights = map_bharat_rights_to_kernel(req.expected_object_type, required_rights);
    req.requester_pid = table->owner_pid;
    req.expected_generation = bh_cap_generation((uint32_t)handle);

    capability_entry_t entry;
    kstatus_t status = cap_validate_ex(table, &req, &entry);

    if (status == K_OK) {
        if (out_result) {
            out_result->allowed = true;
            out_result->status = BHARAT_CAP_OK;
            out_result->descriptor.handle = handle;
            out_result->descriptor.cap_id = entry.id;
            out_result->descriptor.object_type = expected_object_type;
            out_result->descriptor.object_id = entry.object_ref;
            out_result->descriptor.rights = required_rights;
            out_result->descriptor.generation = entry.generation;
            out_result->descriptor.state = entry.state;
        }
        return BHARAT_CAP_OK;
    } else {
        bharat_cap_status_t out_status = BHARAT_CAP_NOT_FOUND;
        switch (status) {
            case K_ERR_CAP_STALE:   out_status = BHARAT_CAP_STALE; break;
            case K_ERR_CAP_REVOKED: out_status = BHARAT_CAP_REVOKED; break;
            case K_ERR_CAP_DENIED:  out_status = BHARAT_CAP_RIGHTS_DENIED; break;
            default:                out_status = BHARAT_CAP_NOT_FOUND; break;
        }
        if (out_result) {
            out_result->allowed = false;
            out_result->status = out_status;
        }
        return out_status;
    }
}

void __attribute__((weak)) bharat_cap_register_authority_resolver(bharat_cap_authority_resolver_fn_t resolver) {
    (void)resolver;
}
