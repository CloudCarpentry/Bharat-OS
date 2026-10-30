#include "cap_internal.h"
#include "panic.h"
#include "time/ktime.h"

void cap_handle_revoke_req(uint64_t payload, uint32_t source_core) {
  cap_handle_tx_req(payload, source_core);
}

void cap_handle_revoke_ack(uint64_t payload) { cap_handle_tx_ack(payload); }

// Helper to lock multiple tables in total order to prevent ABBA deadlocks.
void cap_lock_tables_sorted(capability_table_t **tables, size_t count) {
  // Simple insertion sort by owner_core, cspace_id, registry_slot
  for (size_t i = 1; i < count; i++) {
    capability_table_t *key = tables[i];
    size_t j = i;
    while (j > 0 && ((tables[j - 1]->owner_core > key->owner_core) ||
                     (tables[j - 1]->owner_core == key->owner_core &&
                      tables[j - 1]->cspace_id > key->cspace_id) ||
                     (tables[j - 1]->owner_core == key->owner_core &&
                      tables[j - 1]->cspace_id == key->cspace_id &&
                      tables[j - 1]->registry_slot > key->registry_slot))) {
      tables[j] = tables[j - 1];
      j--;
    }
    tables[j] = key;
  }

  // Lock uniquely
  capability_table_t *last_locked = NULL;
  for (size_t i = 0; i < count; i++) {
    if (tables[i] != last_locked) {
      spin_lock(&tables[i]->lock);
      last_locked = tables[i];
    }
  }
}

void cap_unlock_tables_sorted(capability_table_t **tables, size_t count) {
  // Unlock uniquely in reverse order
  capability_table_t *last_unlocked = NULL;
  for (size_t i = count; i > 0; i--) {
    if (tables[i - 1] != last_unlocked) {
      spin_unlock(&tables[i - 1]->lock);
      last_unlocked = tables[i - 1];
    }
  }
}

int cap_table_revoke(capability_table_t *table, uint32_t cap_id) {
  if (!BHARAT_PTR_NON_NULL(table) || cap_id == 0U) {
    return -1;
  }

  uint32_t current_core = hal_cpu_get_id();

  __attribute__((weak)) extern bool g_pmm_initialized;
  bool pmm_is_initialized = &g_pmm_initialized ? g_pmm_initialized : true;

  uint8_t tx_slot = 0;
  bh_cap_tx_entry_t *tx = NULL;

  if (current_core < MAX_CPUS && pmm_is_initialized) {
    tx = cap_tx_alloc(current_core, BH_CAP_TX_OP_REVOKE, &tx_slot);
    if (!tx) {
        return -6;
    }
  }

  uint32_t id_only = cap_id & 0xFFFF;
  uint32_t generation = cap_id >> 16;

  spin_lock(&table->lock);

  uint32_t root_slot = UINT32_MAX;
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
    if (table->entries[i].in_use != 0U && table->entries[i].id == id_only) {
      if (!bh_cap_generation_matches(table->entries[i].generation,
                                     generation)) {
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
    if (tx) cap_tx_release(current_core, tx_slot);
    return -2;
  }

  if (root_slot >= BHARAT_ARRAY_SIZE(table->entries)) {
    spin_unlock(&table->lock);
    if (tx) cap_tx_release(current_core, tx_slot);
    return -2;
  }

  capability_entry_t *root_entry = &table->entries[root_slot];
  uint32_t root_gen = root_entry->generation;

  bh_cap_locator_t parent_locator = root_entry->parent;
  capability_table_t *parent_table = cap_locator_resolve_table(&parent_locator);
  uint32_t parent_slot = root_entry->parent.slot;
  uint32_t parent_gen = root_entry->parent.generation;

  spin_unlock(&table->lock);

  if (parent_table) {
    capability_table_t *tables_to_lock[16];
    size_t num_tables = 0;

    tables_to_lock[num_tables++] = table;
    tables_to_lock[num_tables++] = parent_table;

    // Scan sibling chain to collect any intermediate sibling tables
    spin_lock(&parent_table->lock);
    if (parent_slot < BHARAT_ARRAY_SIZE(parent_table->entries)) {
      capability_entry_t *p_ent = &parent_table->entries[parent_slot];
      if (p_ent->in_use != 0U && p_ent->generation == parent_gen) {
        bh_cap_locator_t s = p_ent->first_child;
        while (!cap_locator_is_null(&s) && num_tables < 16) {
          capability_table_t *st = cap_locator_resolve_table(&s);
          if (st) {
            bool found = false;
            for (size_t k = 0; k < num_tables; k++) {
              if (tables_to_lock[k] == st) {
                found = true;
                break;
              }
            }
            if (!found) {
              tables_to_lock[num_tables++] = st;
            }
          }
          if (st && s.slot < BHARAT_ARRAY_SIZE(st->entries)) {
            s = st->entries[s.slot].next_sibling;
          } else {
            break;
          }
        }
      }
    }
    spin_unlock(&parent_table->lock);

    cap_lock_tables_sorted(tables_to_lock, num_tables);

    // Verify root hasn't been reallocated
    if (root_entry->in_use != 0U && root_entry->generation == root_gen) {
      // Verify parent hasn't been reallocated
      if (parent_slot >= BHARAT_ARRAY_SIZE(parent_table->entries)) {
        cap_unlock_tables_sorted(tables_to_lock, num_tables);
        if (tx) cap_tx_release(current_core, tx_slot);
        return -2;
      }
      capability_entry_t *parent = &parent_table->entries[parent_slot];
      if (parent->in_use != 0U && parent->generation == parent_gen) {
        bh_cap_locator_t sibling = parent->first_child;
        bh_cap_locator_t prev = cap_locator_null();

        while (!cap_locator_is_null(&sibling)) {
          capability_table_t *sibling_table =
              cap_locator_resolve_table(&sibling);
          if (sibling_table == NULL) {
            break;
          }
          if (sibling.slot >= BHARAT_ARRAY_SIZE(sibling_table->entries)) {
            break;
          }
          if (sibling_table == table && sibling.slot == root_slot &&
              sibling.generation == root_gen) {
            if (!cap_locator_is_null(&prev)) {
              capability_table_t *prev_table = cap_locator_resolve_table(&prev);
              if (prev_table && prev.slot < BHARAT_ARRAY_SIZE(prev_table->entries)) {
                prev_table->entries[prev.slot].next_sibling =
                    root_entry->next_sibling;
              }
            } else {
              parent->first_child = root_entry->next_sibling;
            }
            break;
          }

          prev = sibling;
          sibling = sibling_table->entries[sibling.slot].next_sibling;
        }
      }
    }

    cap_unlock_tables_sorted(tables_to_lock, num_tables);
  }

  // Stackless tree walk to freeze and mark all children as REVOKING
  bh_cap_locator_t current = cap_locator_make(table, root_slot, root_gen, 0);

  while (!cap_locator_is_null(&current)) {
    capability_table_t *curr_table = cap_locator_resolve_table(&current);
    if (curr_table == NULL) break;

    spin_lock(&curr_table->lock);

    if (current.slot >= BHARAT_ARRAY_SIZE(curr_table->entries)) {
      spin_unlock(&curr_table->lock);
      break;
    }

    capability_entry_t *cap = &curr_table->entries[current.slot];

    bool is_stale = (cap->in_use == 0U || cap->generation != current.generation);

    bh_cap_locator_t child = cap_locator_null();
    bh_cap_locator_t sibling = cap_locator_null();
    bh_cap_locator_t parent_loc = cap_locator_null();

    if (!is_stale) {
        cap->state = CAP_STATE_REVOKING;
        child = cap->first_child;
        sibling = cap->next_sibling;
        parent_loc = cap->parent;
    }

    spin_unlock(&curr_table->lock);

    if (is_stale) {
        if (tx) {
            atomic_set(&tx->state, BH_CAP_TX_ABORTED);
            cap_tx_release(current_core, tx_slot);
        }
        return -3; // Traversal failed cleanly before mutation
    }

    if (!cap_locator_is_null(&child)) {
        current = child;
    } else if (!cap_locator_is_null(&sibling) && (curr_table != table || current.slot != root_slot)) {
        current = sibling;
    } else {
        // Go back up to find next sibling of some ancestor
        current = parent_loc;
        while (!cap_locator_is_null(&current)) {
            if (current.owner_core == table->owner_core &&
                current.cspace_id == table->cspace_id &&
                current.slot == root_slot) {
                current = cap_locator_null();
                break;
            }

            capability_table_t *p_table = cap_locator_resolve_table(&current);
            if (!p_table) {
                current = cap_locator_null();
                break;
            }

            spin_lock(&p_table->lock);
            if (current.slot < BHARAT_ARRAY_SIZE(p_table->entries)) {
                 capability_entry_t *p_cap = &p_table->entries[current.slot];
                 if (p_cap->in_use != 0U && p_cap->generation == current.generation) {
                     bh_cap_locator_t p_next = p_cap->next_sibling;
                     bh_cap_locator_t p_parent = p_cap->parent;
                     spin_unlock(&p_table->lock);
                     if (!cap_locator_is_null(&p_next)) {
                         current = p_next;
                         break;
                     } else {
                         current = p_parent;
                         continue;
                     }
                 }
            }
            spin_unlock(&p_table->lock);
            current = cap_locator_null();
        }
    }
  }

  // Increment revocation epoch BEFORE broadcast
  spin_lock(&table->lock);
  uint64_t epoch = 0;
  if (root_entry->in_use != 0U && root_entry->generation == root_gen) {
    root_entry->revocation_epoch++;
    epoch = root_entry->revocation_epoch;
  }
  spin_unlock(&table->lock);

  if (tx) {
      tx->target =
          cap_locator_make(table, root_slot, root_gen, (uint32_t)epoch);
      tx->revocation_epoch = epoch;
      tx->target_mask = 0;
      tx->ack_mask = 0;
      tx->result = 0;

      for (uint32_t c = 0; c < MAX_CPUS; c++) {
        if (c != current_core &&
            urpc_channel_get_state(c) == URPC_CHANNEL_BOUND) {
          tx->target_mask |= (1U << c);
        }
      }

      __asm__ volatile("" : : : "memory");
      atomic_set(&tx->state, BH_CAP_TX_PUBLISHED);

      for (uint32_t c = 0; c < MAX_CPUS; c++) {
        if ((tx->target_mask & (1U << c)) != 0) {
          uint64_t req_payload =
              cap_tx_pack_req((uint8_t)current_core, tx_slot, tx->generation,
                              (uint8_t)BH_CAP_TX_OP_REVOKE_PREPARE);
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
        // Quarantine the root in CAP_STATE_QUARANTINED and fail closed without
        // freeing the slot while remote cores may still hold authority.
        spin_lock(&table->lock);
        if (root_entry->in_use != 0U && root_entry->generation == root_gen) {
          root_entry->state = CAP_STATE_QUARANTINED;
        }
        spin_unlock(&table->lock);
        return K_ERR_TIMEOUT;
      }

      atomic_set(&tx->state, BH_CAP_TX_COMMITTED);
      cap_tx_release(current_core, tx_slot);
    }

  // Pass 2: Free nodes safely using stateful stackless post-order traversal
  current = cap_locator_make(table, root_slot, root_gen, 0);

  // Track direction to avoid dereferencing freed `previous` pointers
  typedef enum { DIR_DOWN, DIR_UP } traversal_dir_t;
  traversal_dir_t dir = DIR_DOWN;

  while (!cap_locator_is_null(&current)) {
    capability_table_t *curr_table = cap_locator_resolve_table(&current);
    if (!curr_table) break;

    spin_lock(&curr_table->lock);

    if (current.slot >= BHARAT_ARRAY_SIZE(curr_table->entries)) {
        spin_unlock(&curr_table->lock);
        break;
    }

    capability_entry_t *cap = &curr_table->entries[current.slot];

    // Prevent cross-core corruption/double-free on reallocated capabilities
    if (cap->in_use == 0U || cap->generation != current.generation) {
        spin_unlock(&curr_table->lock);
        break;
    }

    bh_cap_locator_t child = cap->first_child;
    bh_cap_locator_t parent_loc = cap->parent;
    bh_cap_locator_t sibling = cap->next_sibling;

    // If we are moving down the tree and there is a child, descend to the child.
    if (dir == DIR_DOWN && !cap_locator_is_null(&child)) {
        spin_unlock(&curr_table->lock);
        current = child;
        continue; // Keep DIR_DOWN
    }

    // Otherwise, we are at a leaf or coming up from children. Wait for active pins if any.
    if (cap->active_pins != 0U) {
        spin_unlock(&curr_table->lock);
        bh_kdeadline_t pin_deadline = bh_deadline_after_ns(5 * BH_KTIME_NS_PER_MS);
        bool pin_timed_out = false;
        while (true) {
            spin_lock(&curr_table->lock);
            bool drained = (cap->in_use == 0U || cap->generation != current.generation || cap->active_pins == 0U);
            spin_unlock(&curr_table->lock);
            if (drained) break;
            if (bh_deadline_expired(pin_deadline)) {
                pin_timed_out = true;
                break;
            }
            extern void arch_cpu_relax(void);
            arch_cpu_relax();
        }
        spin_lock(&curr_table->lock);
        if (cap->in_use == 0U || cap->generation != current.generation) {
            spin_unlock(&curr_table->lock);
            break;
        }
        if (pin_timed_out && cap->active_pins != 0U) {
            cap->state = CAP_STATE_QUARANTINED;
            spin_unlock(&curr_table->lock);
            return K_ERR_TIMEOUT;
        }
    }

    cap_type_t released_type = cap->type;
    uint64_t released_obj_ref = cap->object_ref;
    uint64_t released_rights = cap->rights;

    // We can now safely free this node.
    cap->rights = 0U;
    cap->flags = 0U;
    cap->object_ref = 0U;
    cap->active_pins = 0U;
    cap->scope_id = 0U;

    cap->parent = cap_locator_null();
    cap->first_child = cap_locator_null();
    cap->next_sibling = cap_locator_null();

    cap->generation++;
    cap->state = CAP_STATE_FREE;
    cap->in_use = 0U;
    bh_id_allocator_free(&curr_table->id_allocator, current.slot);

    spin_unlock(&curr_table->lock);

    cap_endpoint_ref_release(released_type, released_obj_ref, released_rights);

    // If we just processed the root, we are completely done.
    if (current.owner_core == table->owner_core &&
        current.cspace_id == table->cspace_id &&
        current.slot == root_slot) {
        break;
    }

    // Move to next sibling if it exists
    if (!cap_locator_is_null(&sibling)) {
        current = sibling;
        dir = DIR_DOWN; // Reset direction for the new sibling subtree
    } else {
        // No siblings left, move up to parent
        current = parent_loc;
        dir = DIR_UP; // We are ascending, so don't descend into its children again
    }
  }

  // If a timeout occurred, we did locally clean up but must still report it to the caller
  if (tx && atomic_get(&tx->state) == BH_CAP_TX_ABORTED) {
      return K_ERR_TIMEOUT;
  }

  return 0;
}
