#include "cap_internal.h"
#include "panic.h"
#include "time/ktime.h"

void cap_handle_revoke_req(uint64_t payload, uint32_t source_core) {
    cap_handle_tx_req(payload, source_core);
}

void cap_handle_revoke_ack(uint64_t payload) {
    cap_handle_tx_ack(payload);
}

// Helper to lock multiple tables in total order to prevent ABBA deadlocks.
void cap_lock_tables_sorted(capability_table_t** tables, size_t count) {
    // Simple insertion sort by owner_core, cspace_id, registry_slot
    for (size_t i = 1; i < count; i++) {
        capability_table_t* key = tables[i];
        int j = i - 1;
        while (j >= 0 && ((tables[j]->owner_core > key->owner_core) ||
                          (tables[j]->owner_core == key->owner_core && tables[j]->cspace_id > key->cspace_id) ||
                          (tables[j]->owner_core == key->owner_core && tables[j]->cspace_id == key->cspace_id && tables[j]->registry_slot > key->registry_slot))) {
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

int cap_table_revoke(capability_table_t* table, uint32_t cap_id) {
    if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
        return -1;
    }

    uint32_t id_only = cap_id & 0xFFFF;
    uint32_t generation = cap_id >> 16;

    // Iterative tree walk to revoke children safely.
    bh_cap_locator_t stack[64];
    size_t sp = 0;

    spin_lock(&table->lock);

    uint32_t root_slot = UINT32_MAX;
    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
        if (table->entries[i].in_use != 0U && table->entries[i].id == id_only) {
            if (!bh_cap_generation_matches(table->entries[i].generation, generation)) {
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

    bh_cap_locator_t parent_locator = root_entry->parent;
    capability_table_t* parent_table = cap_locator_resolve_table(&parent_locator);
    uint32_t parent_slot = root_entry->parent.slot;
    uint32_t parent_gen = root_entry->parent.generation;

    spin_unlock(&table->lock);

    if (parent_table) {
        capability_table_t* tables_to_lock[16];
        size_t num_tables = 0;

        tables_to_lock[num_tables++] = table;
        tables_to_lock[num_tables++] = parent_table;

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
                bh_cap_locator_t sibling = parent->first_child;
                bh_cap_locator_t prev = cap_locator_null();

                while (!cap_locator_is_null(&sibling)) {
                    capability_table_t *sibling_table = cap_locator_resolve_table(&sibling);
                    if (sibling_table != table && sibling_table != parent_table) {
                        break;
                    }
                    if (sibling.slot >= BHARAT_ARRAY_SIZE(sibling_table->entries)) {
                        break;
                    }
                    if (sibling_table == table && sibling.slot == root_slot && sibling.generation == root_gen) {
                        if (!cap_locator_is_null(&prev)) {
                            capability_table_t *prev_table = cap_locator_resolve_table(&prev);
                            if (prev_table == table) {
                                table->entries[prev.slot].next_sibling = root_entry->next_sibling;
                            } else if (prev_table == parent_table) {
                                parent_table->entries[prev.slot].next_sibling = root_entry->next_sibling;
                            }
                        } else {
                            parent->first_child = root_entry->next_sibling;
                        }
                        break;
                    }

                    prev = sibling;
                    if (sibling_table == table) {
                        sibling = table->entries[sibling.slot].next_sibling;
                    } else if (sibling_table == parent_table) {
                        sibling = parent_table->entries[sibling.slot].next_sibling;
                    } else {
                        break;
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
    stack[sp] = cap_locator_make(table, root_slot, root_gen, (uint32_t)epoch);
    sp++;

    uint32_t current_core = hal_cpu_get_id();

    __attribute__((weak)) extern bool g_pmm_initialized;
    bool pmm_is_initialized = &g_pmm_initialized ? g_pmm_initialized : true;

    if (current_core < MAX_CPUS && pmm_is_initialized) {
        uint8_t tx_slot = 0;
        bh_cap_tx_entry_t *tx = cap_tx_alloc(current_core, BH_CAP_TX_OP_REVOKE, &tx_slot);
        if (tx) {
            tx->target = cap_locator_make(table, root_slot, root_gen, (uint32_t)epoch);
            tx->revocation_epoch = epoch;
            tx->target_mask = 0;
            tx->ack_mask = 0;
            tx->result = 0;

            for (uint32_t c = 0; c < MAX_CPUS; c++) {
                if (c != current_core && urpc_channel_get_state(c) == URPC_CHANNEL_BOUND) {
                    tx->target_mask |= (1U << c);
                }
            }

            __asm__ volatile("" : : : "memory");
            atomic_set(&tx->state, BH_CAP_TX_PUBLISHED);

            for (uint32_t c = 0; c < MAX_CPUS; c++) {
                if ((tx->target_mask & (1U << c)) != 0) {
                    uint64_t req_payload = cap_tx_pack_req((uint8_t)current_core, tx_slot, tx->generation, (uint8_t)BH_CAP_TX_OP_REVOKE);
                    urpc_bootstrap_send(c, urpc_pack_msg(URPC_CAP_REVOKE, req_payload));
                }
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
                cap_tx_release(current_core, tx_slot);
                kernel_panic("Capability Revocation Timeout: Bounded synchronization failed! Halted to prevent security breach.");
            }

            atomic_set(&tx->state, BH_CAP_TX_COMMITTED);
            cap_tx_release(current_core, tx_slot);
        }
    }

    while (sp > 0) {
        bh_cap_locator_t frame = stack[--sp];
        capability_table_t *frame_table = cap_locator_resolve_table(&frame);

        if (frame_table == NULL) {
            continue;
        }

        spin_lock(&frame_table->lock);

        if (frame.slot >= BHARAT_ARRAY_SIZE(frame_table->entries)) {
            spin_unlock(&frame_table->lock);
            continue;
        }

        capability_entry_t* cap = &frame_table->entries[frame.slot];
        if (cap->in_use == 0U || cap->generation != frame.generation) {
            spin_unlock(&frame_table->lock);
            continue;
        }

        if (frame_table != table || frame.slot != root_slot) {
            if (!cap_locator_is_null(&cap->next_sibling)) {
                if (sp >= 64) {
                    spin_unlock(&frame_table->lock);
                    return -3; // bounded-stack overflow
                }
                stack[sp] = cap->next_sibling;
                sp++;
            }
        }

        if (!cap_locator_is_null(&cap->first_child)) {
            if (sp >= 64) {
                spin_unlock(&frame_table->lock);
                return -3; // bounded-stack overflow
            }
            stack[sp] = cap->first_child;
            sp++;
        }

        cap->rights = 0U;
        cap->flags = 0U;
        cap->object_ref = 0U;

        cap->parent = cap_locator_null();
        cap->first_child = cap_locator_null();
        cap->next_sibling = cap_locator_null();

        cap->generation++;
        cap->state = CAP_STATE_FREE;
        cap->in_use = 0U;
        bh_id_allocator_free(&frame_table->id_allocator, frame.slot);

        spin_unlock(&frame_table->lock);
    }

    return 0;
}
