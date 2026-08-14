#include "cap_internal.h"

bh_cap_cspace_registry_entry_t
    g_cap_cspace_registry[MAX_CPUS][BH_CAP_CSPACES_PER_CORE];
capability_table_t g_cap_bootstrap_cspaces[MAX_CPUS];
bool g_cap_bootstrap_cspaces_used[MAX_CPUS];

cap_delegate_req_t g_cap_delegations[MAX_CPUS];

_Static_assert(MAX_CPUS <= (UINT32_C(1) << BH_CAP_CSPACE_OWNER_BITS),
               "CSpace identity must encode every owner core");
_Static_assert(BH_CAP_CSPACES_PER_CORE <=
                   (UINT32_C(1) << BH_CAP_CSPACE_SLOT_BITS),
               "CSpace identity must encode every registry slot");
_Static_assert(sizeof(((cap_delegate_req_t *)0)->src) == sizeof(bh_cap_locator_t),
               "delegation source must be a locator");
_Static_assert(sizeof(((cap_delegate_req_t *)0)->dst) == sizeof(bh_cap_locator_t),
               "delegation destination must be a locator");

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

capability_table_t* cap_table_create(void) {
    uint32_t owner_core = hal_cpu_get_id();
    if (owner_core >= MAX_CPUS) {
        return NULL;
    }

    for (size_t i = 0; i < BH_CAP_CSPACES_PER_CORE; ++i) {
        bh_cap_cspace_registry_entry_t *registered =
            &g_cap_cspace_registry[owner_core][i];
        if (registered->table == NULL) {
            capability_table_t *t = kmalloc(sizeof(*t));
            if (t == NULL) {
                if (g_cap_bootstrap_cspaces_used[owner_core]) {
                    return NULL;
                }
                t = &g_cap_bootstrap_cspaces[owner_core];
                g_cap_bootstrap_cspaces_used[owner_core] = true;
                registered->uses_bootstrap_storage = true;
            } else {
                registered->uses_bootstrap_storage = false;
            }
            memset(t, 0, sizeof(*t));
            registered->generation++;
            if (registered->generation == 0U) {
                registered->generation = 1U;
            }
            spin_lock_init(&t->lock);
            bh_id_allocator_init(&t->id_allocator, t->id_bitmap, BHARAT_ARRAY_SIZE(t->entries));
            t->numa_node = 0U; // Placeholder, would be set to actual node in full implementation
            t->cspace_id = cap_cspace_id_make(owner_core, (uint32_t)i,
                                               registered->generation);
            t->owner_core = (uint16_t)owner_core;
            t->registry_slot = (uint16_t)i;
            t->owner_pid = 0U;
            /*@
              loop invariant 0 <= j <= BHARAT_ARRAY_SIZE(t->entries);
              loop assigns j, t->entries[0..BHARAT_ARRAY_SIZE(t->entries)-1].in_use;
              loop variant BHARAT_ARRAY_SIZE(t->entries) - j;
            */
            for (size_t j = 0; j < BHARAT_ARRAY_SIZE(t->entries); ++j) {
                t->entries[j].in_use = 0U;
                t->entries[j].parent = cap_locator_null();
                t->entries[j].first_child = cap_locator_null();
                t->entries[j].next_sibling = cap_locator_null();
            }
            /* Publish only after the complete CSpace has been initialized. */
            registered->table = t;
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
    uint32_t owner_core = table->owner_core;
    uint32_t slot = table->registry_slot;
    if (owner_core >= MAX_CPUS || slot >= BH_CAP_CSPACES_PER_CORE ||
        g_cap_cspace_registry[owner_core][slot].table != table) {
        return;
    }
    /* Owner-local unpublish makes all old locators fail before storage release. */
    bh_cap_cspace_registry_entry_t *registered =
        &g_cap_cspace_registry[owner_core][slot];
    registered->table = NULL;
    if (registered->uses_bootstrap_storage) {
        g_cap_bootstrap_cspaces_used[owner_core] = false;
        registered->uses_bootstrap_storage = false;
    } else {
        kfree(table);
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
            e->parent = cap_locator_null();
            e->first_child = cap_locator_null();
            e->next_sibling = cap_locator_null();
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

    capability_entry_t found_entry = {0};
    int ret = -4;

    // table must be cast to non-const for spinlock.
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
            if (!bh_cap_generation_matches(e->generation, generation)) {
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
            if (!bh_cap_generation_matches(src->entries[i].generation, generation)) {
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

                dst_entry->parent = cap_locator_make(src, src_slot_idx,
                                                     src_entry->generation,
                                                     (uint32_t)src_entry->revocation_epoch);

                dst_entry->first_child = cap_locator_null();

                dst_entry->next_sibling = src_entry->first_child;

                dst_entry->generation++;
                dst_entry->owner_core = src_entry->owner_core; // Delegate retains owner core unless explicity transferred

                // Inherit distributed instance ID
                dst_entry->instance_id = src_entry->instance_id;
                dst_entry->instance_id.rights_digest = delegated_rights;
                dst_entry->instance_id.slot_gen = dst_entry->generation;
                dst_entry->revocation_epoch = src_entry->revocation_epoch;

                dst_entry->in_use = 1U;

                src_entry->first_child = cap_locator_make(dst, dst_slot_idx,
                                                          dst_entry->generation,
                                                          (uint32_t)dst_entry->revocation_epoch);

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

int cap_table_delegate(capability_table_t* src,
                       capability_table_t* dst,
                       uint32_t cap_id,
                       uint64_t delegated_rights,
                       uint32_t* out_new_cap_id) {
    if (!BHARAT_PTR_NON_NULL(src) || !BHARAT_PTR_NON_NULL(dst) || cap_id == 0U) {
        return -1;
    }

    uint32_t current_core = hal_cpu_get_id();
    uint32_t target_core = dst->owner_core;
    bool destination_is_registered =
        target_core < MAX_CPUS &&
        dst->registry_slot < BH_CAP_CSPACES_PER_CORE &&
        g_cap_cspace_registry[target_core][dst->registry_slot].table == dst &&
        dst->cspace_id != 0U;
    if (!destination_is_registered) {
        return -6;
    }
    if (target_core == current_core) {
        return cap_table_delegate_local(src, dst, cap_id, delegated_rights, out_new_cap_id);
    }
    if (urpc_channel_get_state(target_core) != URPC_CHANNEL_BOUND) {
        return -6;
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
            if (!bh_cap_generation_matches(src->entries[i].generation, generation)) {
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
    req->src = cap_locator_make(src, src_slot_idx, src_entry->generation,
                                (uint32_t)src_entry->revocation_epoch);
    req->dst = cap_locator_make(dst, 0U, 0U, 0U);
    req->type = (uint32_t)src_entry->type;
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
    if (src_entry->in_use != 0U && src_entry->id == id_only &&
        src_entry->generation == req->src.generation &&
        src_entry->state == CAP_STATE_LIVE) {
        src_entry->first_child = cap_locator_make(dst, req->dst_slot,
                                                  req->dst_generation,
                                                  (uint32_t)req->revocation_epoch);

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
                                   ((uint64_t)req->src.slot << 40) |
                                   ((uint64_t)req->src.generation << 24) |
                                   current_core;
        urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_REVOKE, rollback_payload));
    }

    return ret;
}
