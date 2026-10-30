#include "cap_internal.h"
#include "time/ktime.h"

int __attribute__((weak)) ipc_endpoint_acquire_send_cap(uint64_t endpoint_obj_ref) {
    (void)endpoint_obj_ref;
    return 0;
}

int __attribute__((weak)) ipc_endpoint_acquire_recv_cap(uint64_t endpoint_obj_ref) {
    (void)endpoint_obj_ref;
    return 0;
}

int __attribute__((weak)) ipc_endpoint_release_send_cap(uint64_t endpoint_obj_ref) {
    (void)endpoint_obj_ref;
    return 0;
}

int __attribute__((weak)) ipc_endpoint_release_recv_cap(uint64_t endpoint_obj_ref) {
    (void)endpoint_obj_ref;
    return 0;
}

bh_cap_cspace_registry_entry_t
    g_cap_cspace_registry[MAX_CPUS][BH_CAP_CSPACES_PER_CORE];
capability_table_t g_cap_bootstrap_cspaces[MAX_CPUS];
bool g_cap_bootstrap_cspaces_used[MAX_CPUS];

/* Owner-local registry lifetime serialization. Lock order is registry -> CSpace
 * locks in cap_lock_two_tables order. Never enter remote delegation while held.
 * IRQ masking prevents an owner-core teardown from preempting a local install. */
static spinlock_t g_cap_registry_lifetime_lock[MAX_CPUS];

bh_cap_tx_entry_t g_cap_tx_table[MAX_CPUS][BH_CAP_TX_PER_CORE];
spinlock_t g_cap_tx_lock[MAX_CPUS];

_Static_assert(MAX_CPUS <= (UINT32_C(1) << BH_CAP_CSPACE_OWNER_BITS),
               "CSpace identity must encode every owner core");
_Static_assert(BH_CAP_CSPACES_PER_CORE <=
                   (UINT32_C(1) << BH_CAP_CSPACE_SLOT_BITS),
               "CSpace identity must encode every registry slot");
_Static_assert(sizeof(((bh_cap_tx_entry_t *)0)->src) == sizeof(bh_cap_locator_t),
               "transaction source must be a locator");
_Static_assert(sizeof(((bh_cap_tx_entry_t *)0)->dst) == sizeof(bh_cap_locator_t),
               "transaction destination must be a locator");

bh_cap_tx_entry_t *cap_tx_alloc(uint32_t origin_core, uint32_t op, uint8_t *out_slot) {
    if (origin_core >= MAX_CPUS || !out_slot) {
        return NULL;
    }

    spin_lock(&g_cap_tx_lock[origin_core]);

    for (uint32_t i = 0; i < BH_CAP_TX_PER_CORE; i++) {
        bh_cap_tx_entry_t *tx = &g_cap_tx_table[origin_core][i];
        int st = atomic_get(&tx->state);
        if (st == BH_CAP_TX_FREE || st == BH_CAP_TX_COMMITTED || st == BH_CAP_TX_ABORTED) {
            tx->generation++;
            if (tx->generation == 0) {
                tx->generation = 1;
            }
            tx->op = op;
            tx->src = cap_locator_null();
            tx->dst = cap_locator_null();
            tx->target = cap_locator_null();
            tx->revocation_epoch = 0;
            tx->type = 0;
            tx->requested_rights = 0;
            tx->object_ref = 0;
            tx->flags = 0;
            tx->owner_core = origin_core;
            memset(&tx->instance_id, 0, sizeof(tx->instance_id));
            tx->src_first_child = cap_locator_null();
            tx->target_mask = 0;
            tx->ack_mask = 0;
            tx->result = 0;
            tx->remote_cap_id = 0;
            tx->remote_dst_slot = UINT32_MAX;
            tx->remote_dst_gen = 0;

            atomic_set(&tx->state, BH_CAP_TX_PREPARED);
            *out_slot = (uint8_t)i;
            spin_unlock(&g_cap_tx_lock[origin_core]);
            return tx;
        }
    }

    spin_unlock(&g_cap_tx_lock[origin_core]);
    return NULL;
}

void cap_tx_release(uint32_t origin_core, uint8_t slot) {
    if (origin_core >= MAX_CPUS || slot >= BH_CAP_TX_PER_CORE) {
        return;
    }
    bh_cap_tx_entry_t *tx = &g_cap_tx_table[origin_core][slot];
    atomic_set(&tx->state, BH_CAP_TX_FREE);
}

capability_entry_t* cap_find_entry(capability_table_t* table, uint32_t cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return NULL;
    }

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

    uint32_t slot = BH_CAP_CSPACES_PER_CORE;
    uint32_t generation = 0U;
    hal_irq_state_t irq_state;
    spin_lock_irqsave(&g_cap_registry_lifetime_lock[owner_core], &irq_state);
    for (uint32_t i = 0U; i < BH_CAP_CSPACES_PER_CORE; ++i) {
        bh_cap_cspace_registry_entry_t *entry =
            &g_cap_cspace_registry[owner_core][i];
        if (entry->table == NULL && !entry->reserved &&
            entry->generation < BH_CAP_CSPACE_GENERATION_MAX) {
            entry->reserved = true;
            generation = ++entry->generation;
            slot = i;
            break;
        }
    }
    spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);
    if (slot == BH_CAP_CSPACES_PER_CORE) {
        return NULL;
    }

    capability_table_t *table = kmalloc(sizeof(*table));
    bool bootstrap_storage = false;
    if (table == NULL) {
        spin_lock_irqsave(&g_cap_registry_lifetime_lock[owner_core], &irq_state);
        if (!g_cap_bootstrap_cspaces_used[owner_core]) {
            g_cap_bootstrap_cspaces_used[owner_core] = true;
            bootstrap_storage = true;
            table = &g_cap_bootstrap_cspaces[owner_core];
        }
        spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);
        if (table == NULL) {
            spin_lock_irqsave(&g_cap_registry_lifetime_lock[owner_core], &irq_state);
            g_cap_cspace_registry[owner_core][slot].reserved = false;
            spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);
            return NULL;
        }
    }

    memset(table, 0, sizeof(*table));
    spin_lock_init(&table->lock);
    bh_id_allocator_init(&table->id_allocator, table->id_bitmap,
                          BHARAT_ARRAY_SIZE(table->entries));
    table->cspace_id = cap_cspace_id_make(owner_core, slot, generation);
    table->owner_core = (uint16_t)owner_core;
    table->registry_slot = (uint16_t)slot;
    for (size_t i = 0U; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        table->entries[i].parent = cap_locator_null();
        table->entries[i].first_child = cap_locator_null();
        table->entries[i].next_sibling = cap_locator_null();
    }

    spin_lock_irqsave(&g_cap_registry_lifetime_lock[owner_core], &irq_state);
    bh_cap_cspace_registry_entry_t *entry =
        &g_cap_cspace_registry[owner_core][slot];
    entry->uses_bootstrap_storage = bootstrap_storage;
    entry->table = table;
    entry->reserved = false;
    spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);
    bharat_cap_register_authority_resolver(kernel_cap_authority_resolver);
    return table;
}

int cap_table_init_for_process(bh_process_t* proc) {
    if (!BHARAT_PTR_NON_NULL(proc)) {
        return -1;
    }

    capability_table_t* t = cap_table_create();
    if (!t) {
        return -2;
    }

    t->owner_pid = proc->process_id;
    proc->security_sandbox_ctx = t;
    return 0;
}

void cap_table_destroy(capability_table_t* table) {
    if (!table) return;
    uint32_t owner_core = table->owner_core;
    uint32_t slot = table->registry_slot;
    if (owner_core >= MAX_CPUS || slot >= BH_CAP_CSPACES_PER_CORE) {
        return;
    }
    if (owner_core != hal_cpu_get_id()) {
        return; /* CSpace teardown is an owner-core operation. */
    }
    hal_irq_state_t irq_state;
    spin_lock_irqsave(&g_cap_registry_lifetime_lock[owner_core], &irq_state);
    bh_cap_cspace_registry_entry_t *registered =
        &g_cap_cspace_registry[owner_core][slot];
    if (registered->table != table) {
        spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);
        return;
    }
    registered->table = NULL;
    bool bootstrap_storage = registered->uses_bootstrap_storage;
    if (bootstrap_storage) {
        g_cap_bootstrap_cspaces_used[owner_core] = false;
        registered->uses_bootstrap_storage = false;
    }
    spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[owner_core], irq_state);

    for (size_t i = 0U; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        capability_entry_t *e = &table->entries[i];
        if (e->in_use != 0U && e->state == CAP_STATE_LIVE) {
            cap_endpoint_ref_release(e->type, e->object_ref, e->rights);
            e->state = CAP_STATE_FREE;
            e->in_use = 0U;
        }
    }

    if (!bootstrap_storage) {
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

    uint32_t slot_idx;
    if (bh_id_allocator_alloc(&table->id_allocator, &slot_idx) == K_OK) {
            capability_entry_t* e = &table->entries[slot_idx];
            e->id = slot_idx + 1;
            e->state = CAP_STATE_LIVE;
            e->type = type;
            e->rights = rights;
            e->object_ref = object_ref;
            e->flags = 0U;
            e->active_pins = 0U;
            e->scope_id = 0U;
            e->parent = cap_locator_null();
            e->first_child = cap_locator_null();
            e->next_sibling = cap_locator_null();
            e->generation++;

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

    if (ret == 0) {
        cap_endpoint_ref_acquire(type, object_ref, rights);
        if (out_cap_id) {
            *out_cap_id = found_id;
        }
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

    capability_table_t* t = (capability_table_t*)table;
    spin_lock(&t->lock);

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        const capability_entry_t* e = &table->entries[i];
        if (e->in_use != 0U && e->id == id_only) {
            if (!bh_cap_generation_matches(e->generation, generation)) {
                ret = -6;
                break;
            }
            if (e->state != CAP_STATE_LIVE) {
                ret = -7;
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

static int cap_table_delegate_local(capability_table_t* src,
                                    capability_table_t* dst,
                                    uint32_t cap_id,
                                    uint64_t delegated_rights,
                                    uint32_t* out_new_cap_id,
                                    const bh_cap_locator_t *expected) {
    cap_lock_two_tables(src, dst);

    int ret = -2;
    uint32_t src_slot_idx = UINT32_MAX;
    capability_entry_t* src_entry = NULL;

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(src->entries); ++i) {
        if (src->entries[i].in_use != 0U && src->entries[i].id == id_only) {
            if (!bh_cap_generation_matches(src->entries[i].generation, generation)) {
                break;
            }
            if (src->entries[i].state != CAP_STATE_LIVE) {
                break;
            }
            src_entry = &src->entries[i];
            src_slot_idx = (uint32_t)i;
            break;
        }
    }

    if (!src_entry || (expected != NULL &&
        (src_slot_idx != expected->slot ||
         src_entry->generation != expected->generation ||
         src_entry->revocation_epoch != expected->revocation_epoch))) {
        ret = -51;
    } else if ((src_entry->rights & CAP_RIGHT_DELEGATE) == 0U) {
        ret = -52;
    } else if ((src_entry->rights & delegated_rights) != delegated_rights) {
        ret = -53;
    } else if (!cap_rights_valid(src_entry->type, delegated_rights)) {
        ret = -54;
    } else if (delegated_rights == CAP_RIGHT_NONE) {
        ret = -55;
    } else {
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
                dst_entry->active_pins = 0U;
                dst_entry->scope_id = src_entry->scope_id;

                dst_entry->parent = cap_locator_make(src, src_slot_idx,
                                                     src_entry->generation,
                                                     (uint32_t)src_entry->revocation_epoch);

                dst_entry->first_child = cap_locator_null();
                dst_entry->next_sibling = src_entry->first_child;

                dst_entry->generation++;
                dst_entry->owner_core = src_entry->owner_core;

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
    }

    cap_type_t acquired_type = (ret == 0 && src_entry != NULL) ? src_entry->type : CAP_TYPE_NONE;
    uint64_t acquired_obj_ref = (ret == 0 && src_entry != NULL) ? src_entry->object_ref : 0U;

    cap_unlock_two_tables(src, dst);

    if (ret == 0) {
        cap_endpoint_ref_acquire(acquired_type, acquired_obj_ref, delegated_rights);
    }

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
        return cap_table_delegate_local(src, dst, cap_id, delegated_rights, out_new_cap_id, NULL);
    }
    if (urpc_channel_get_state(target_core) != URPC_CHANNEL_BOUND) {
        return -6;
    }

    spin_lock(&src->lock);

    int ret = -2;
    uint32_t src_slot_idx = UINT32_MAX;
    capability_entry_t* src_entry = NULL;

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(src->entries); ++i) {
        if (src->entries[i].in_use != 0U && src->entries[i].id == id_only) {
            if (!bh_cap_generation_matches(src->entries[i].generation, generation)) {
                break;
            }
            if (src->entries[i].state != CAP_STATE_LIVE) {
                break;
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

    uint8_t tx_slot = 0;
    bh_cap_tx_entry_t* tx = cap_tx_alloc(current_core, BH_CAP_TX_OP_DELEGATE, &tx_slot);
    if (!tx) {
        spin_unlock(&src->lock);
        return -6;
    }

    tx->src = cap_locator_make(src, src_slot_idx, src_entry->generation,
                               (uint32_t)src_entry->revocation_epoch);
    tx->dst = cap_locator_make(dst, 0U, 0U, 0U);
    tx->type = (uint32_t)src_entry->type;
    tx->requested_rights = delegated_rights;
    tx->object_ref = src_entry->object_ref;
    tx->flags = src_entry->flags;
    tx->owner_core = src_entry->owner_core;
    tx->instance_id = src_entry->instance_id;
    tx->instance_id.rights_digest = delegated_rights;
    tx->revocation_epoch = src_entry->revocation_epoch;
    tx->src_first_child = src_entry->first_child;
    tx->target_mask = (1U << target_core);
    tx->ack_mask = 0U;
    tx->result = 0;
    tx->remote_cap_id = 0;
    tx->remote_dst_slot = UINT32_MAX;
    tx->remote_dst_gen = 0;

    spin_unlock(&src->lock);

    __asm__ volatile("" : : : "memory");
    atomic_set(&tx->state, BH_CAP_TX_PUBLISHED);

    uint64_t req_payload = cap_tx_pack_req((uint8_t)current_core, tx_slot, tx->generation, (uint8_t)BH_CAP_TX_OP_DELEGATE);
    if (urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_DELEGATE_REQ, req_payload)) != 0) {
        atomic_set(&tx->state, BH_CAP_TX_ABORTED);
        cap_tx_release(current_core, tx_slot);
        return -6;
    }

    bh_kdeadline_t deadline = bh_deadline_after_ns(10 * BH_KTIME_NS_PER_MS);
    bool timed_out = false;

    while ((tx->ack_mask & tx->target_mask) != tx->target_mask) {
        if (bh_deadline_expired(deadline)) {
            timed_out = true;
            break;
        }
        extern void arch_cpu_relax(void);
        arch_cpu_relax();
    }

    if (timed_out) {
        atomic_set(&tx->state, BH_CAP_TX_ABORTED);
        uint64_t rollback_payload = cap_tx_pack_req((uint8_t)current_core, tx_slot, tx->generation, (uint8_t)BH_CAP_TX_OP_ROLLBACK);
        urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_REVOKE, rollback_payload));
        cap_tx_release(current_core, tx_slot);
        return K_ERR_TIMEOUT;
    }

    if (tx->result != 0) {
        int32_t res = tx->result;
        atomic_set(&tx->state, BH_CAP_TX_ABORTED);
        cap_tx_release(current_core, tx_slot);
        return res;
    }

    spin_lock(&src->lock);

    src_entry = &src->entries[src_slot_idx];
    if (src_entry->in_use != 0U && src_entry->id == id_only &&
        src_entry->generation == tx->src.generation &&
        src_entry->state == CAP_STATE_LIVE) {
        src_entry->first_child = cap_locator_make(dst, tx->remote_dst_slot,
                                                  tx->remote_dst_gen,
                                                  (uint32_t)tx->revocation_epoch);

        if (out_new_cap_id) {
            *out_new_cap_id = tx->remote_cap_id | (tx->remote_dst_gen << 16);
        }
        atomic_set(&tx->state, BH_CAP_TX_COMMITTED);
        ret = 0;
    } else {
        atomic_set(&tx->state, BH_CAP_TX_ABORTED);
        ret = -7;
    }

    spin_unlock(&src->lock);

    if (ret != 0) {
        uint64_t rollback_payload = cap_tx_pack_req((uint8_t)current_core, tx_slot, tx->generation, (uint8_t)BH_CAP_TX_OP_ROLLBACK);
        urpc_bootstrap_send(target_core, urpc_pack_msg(URPC_CAP_REVOKE, rollback_payload));
    }

    cap_tx_release(current_core, tx_slot);
    return ret;
}

kstatus_t bh_cap_transfer_capture(capability_table_t *table, uint32_t cap_id,
                                   cap_rights_mask_t rights,
                                   bh_cap_locator_t *out_locator) {
    if (table == NULL || out_locator == NULL || rights == CAP_RIGHT_NONE) {
        return K_ERR_INVALID_ARG;
    }
    *out_locator = cap_locator_null();
    uint32_t core = hal_cpu_get_id();
    if (core >= MAX_CPUS || table->owner_core != core ||
        table->registry_slot >= BH_CAP_CSPACES_PER_CORE) {
        return K_ERR_CAP_OWNERSHIP;
    }
    hal_irq_state_t irq_state;
    spin_lock_irqsave(&g_cap_registry_lifetime_lock[core], &irq_state);
    kstatus_t result = K_ERR_CAP_STALE;
    if (g_cap_cspace_registry[core][table->registry_slot].table == table &&
        table->cspace_id != 0U) {
        spin_lock(&table->lock);
        for (uint32_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
            capability_entry_t *entry = &table->entries[i];
            if (entry->in_use != 0U && entry->id == bh_cap_index(cap_id) &&
                bh_cap_generation_matches(entry->generation, bh_cap_generation(cap_id)) &&
                entry->state == CAP_STATE_LIVE &&
                (entry->rights & CAP_RIGHT_DELEGATE) != 0U &&
                cap_can_transfer(entry->type, entry->rights, rights)) {
                *out_locator = cap_locator_make(table, i, entry->generation,
                                                (uint32_t)entry->revocation_epoch);
                result = K_OK;
                break;
            }
        }
        spin_unlock(&table->lock);
    }
    spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[core], irq_state);
    return result;
}

kstatus_t bh_cap_transfer_install(const bh_cap_locator_t *locator,
                                   capability_table_t *destination,
                                   cap_rights_mask_t rights,
                                   uint32_t *out_cap_id) {
    if (locator == NULL || destination == NULL || out_cap_id == NULL ||
        rights == CAP_RIGHT_NONE) {
        return K_ERR_INVALID_ARG;
    }
    *out_cap_id = 0U;
    uint32_t core = hal_cpu_get_id();
    if (core >= MAX_CPUS || locator->owner_core != core ||
        destination->owner_core != core) {
        return K_ERR_IPC_CROSS_CORE;
    }
    hal_irq_state_t irq_state;
    spin_lock_irqsave(&g_cap_registry_lifetime_lock[core], &irq_state);
    capability_table_t *source = cap_locator_resolve_table(locator);
    kstatus_t result = K_ERR_CAP_STALE;
    if (source != NULL && locator->slot < BHARAT_ARRAY_SIZE(source->entries) &&
        destination->registry_slot < BH_CAP_CSPACES_PER_CORE &&
        g_cap_cspace_registry[core][destination->registry_slot].table == destination) {
        uint32_t cap_id = (uint32_t)(locator->slot + 1U) |
                          (locator->generation << BH_CAP_GEN_SHIFT);
        int delegated = cap_table_delegate_local(source, destination, cap_id,
                                                 rights, out_cap_id, locator);
        result = delegated == 0 ? K_OK :
                 delegated == -2 ? K_ERR_NO_RESOURCES : K_ERR_CAP_STALE;
    }
    spin_unlock_irqrestore(&g_cap_registry_lifetime_lock[core], irq_state);
    return result;
}
