#include "cap_internal.h"

void cap_handle_tx_req(uint64_t payload, uint32_t source_core) {
    uint8_t origin_core = 0;
    uint8_t slot = 0;
    uint32_t generation = 0;
    uint8_t op = 0;

    cap_tx_unpack_req(payload, &origin_core, &slot, &generation, &op);

    if (origin_core >= MAX_CPUS || slot >= BH_CAP_TX_PER_CORE) {
        return;
    }

    bh_cap_tx_entry_t *tx = &g_cap_tx_table[origin_core][slot];
    if (tx->generation != generation) {
        return;
    }

    int state = atomic_get(&tx->state);
    if (state != BH_CAP_TX_PUBLISHED && state != BH_CAP_TX_ABORTED) {
        return;
    }

    uint32_t current_core = hal_cpu_get_id();

    if (op == BH_CAP_TX_OP_DELEGATE) {
        bh_cap_locator_t dst_loc = tx->dst;
        capability_table_t* dst = cap_locator_resolve_table(&dst_loc);
        if (!dst) {
            uint64_t ack_payload = cap_tx_pack_ack(origin_core, slot, generation, (uint8_t)current_core, -1);
            urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_DELEGATE_ACK, ack_payload));
            return;
        }

        if (dst_loc.owner_core != current_core || dst->owner_core != current_core) {
            uint64_t ack_payload = cap_tx_pack_ack(origin_core, slot, generation, (uint8_t)current_core, -2);
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
            dst_entry->type = (cap_type_t)tx->type;
            dst_entry->rights = tx->requested_rights;
            dst_entry->object_ref = tx->object_ref;
            dst_entry->flags = tx->flags;
            dst_entry->owner_core = tx->owner_core;

            dst_entry->instance_id = tx->instance_id;
            dst_entry->revocation_epoch = tx->revocation_epoch;

            dst_entry->parent = tx->src;
            dst_entry->first_child = cap_locator_null();
            dst_entry->next_sibling = tx->src_first_child;

            dst_entry->generation++;
            dst_entry->instance_id.slot_gen = dst_entry->generation;
            dst_entry->in_use = 1U;

            found_id = dst_entry->id;
            dst_slot = dst_slot_idx;
            dst_generation = dst_entry->generation;
            ret = 0;

            tx->remote_cap_id = found_id;
            tx->remote_dst_slot = dst_slot;
            tx->remote_dst_gen = dst_generation;
        }

        spin_unlock(&dst->lock);

        __asm__ volatile("" : : : "memory");
        uint64_t ack_payload = cap_tx_pack_ack(origin_core, slot, generation, (uint8_t)current_core, (int8_t)ret);
        urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_DELEGATE_ACK, ack_payload));
    } else if (op == BH_CAP_TX_OP_ROLLBACK) {
        // Rollback remote capability
        if (tx->remote_dst_slot != UINT32_MAX) {
            capability_table_t* dst = cap_locator_resolve_table(&tx->dst);
            if (dst && tx->remote_dst_slot < BHARAT_ARRAY_SIZE(dst->entries)) {
                spin_lock(&dst->lock);
                capability_entry_t *e = &dst->entries[tx->remote_dst_slot];
                if (e->in_use != 0U && e->generation == tx->remote_dst_gen) {
                    e->rights = 0U;
                    e->flags = 0U;
                    e->object_ref = 0U;
                    e->parent = cap_locator_null();
                    e->first_child = cap_locator_null();
                    e->next_sibling = cap_locator_null();
                    e->generation++;
                    e->state = CAP_STATE_FREE;
                    e->in_use = 0U;
                    bh_id_allocator_free(&dst->id_allocator, tx->remote_dst_slot);
                }
                spin_unlock(&dst->lock);
            }
        }
        uint64_t ack_payload = cap_tx_pack_ack(origin_core, slot, generation, (uint8_t)current_core, 0);
        urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_REVOKE_ACK, ack_payload));
    } else if (op == BH_CAP_TX_OP_REVOKE) {
        uint32_t revocation_epoch = (uint32_t)tx->revocation_epoch;
        uint32_t src_slot = tx->target.slot;
        uint32_t src_generation = tx->target.generation;

        for (size_t cspace = 0; cspace < BH_CAP_CSPACES_PER_CORE; ++cspace) {
            capability_table_t *table = g_cap_cspace_registry[current_core][cspace].table;
            if (table == NULL) continue;

            spin_lock(&table->lock);
            for (size_t i = 0; i < BHARAT_ARRAY_SIZE(table->entries); ++i) {
                capability_entry_t *entry = &table->entries[i];
                if (entry->in_use != 0U && entry->state == CAP_STATE_LIVE) {
                    bool is_descendant =
                        entry->parent.owner_core == origin_core &&
                        entry->parent.slot == src_slot &&
                        entry->parent.generation == src_generation &&
                        revocation_epoch >= entry->parent.revocation_epoch;

                    if (is_descendant) {
                        entry->rights = 0U;
                        entry->flags = 0U;
                        entry->object_ref = 0U;
                        entry->parent = cap_locator_null();
                        entry->first_child = cap_locator_null();
                        entry->next_sibling = cap_locator_null();
                        entry->generation++;
                        entry->state = CAP_STATE_FREE;
                        entry->in_use = 0U;
                        bh_id_allocator_free(&table->id_allocator, (uint32_t)i);
                    }
                }
            }
            spin_unlock(&table->lock);
        }

        uint64_t ack_payload = cap_tx_pack_ack(origin_core, slot, generation, (uint8_t)current_core, 0);
        urpc_bootstrap_send(source_core, urpc_pack_msg(URPC_CAP_REVOKE_ACK, ack_payload));
    }
}

void cap_handle_tx_ack(uint64_t payload) {
    uint8_t origin_core = 0;
    uint8_t slot = 0;
    uint32_t generation = 0;
    uint8_t responder_core = 0;
    int8_t status = 0;

    cap_tx_unpack_ack(payload, &origin_core, &slot, &generation, &responder_core, &status);

    if (origin_core >= MAX_CPUS || origin_core != hal_cpu_get_id() ||
        slot >= BH_CAP_TX_PER_CORE || responder_core >= MAX_CPUS) {
        return;
    }

    bh_cap_tx_entry_t *tx = &g_cap_tx_table[origin_core][slot];

    // Stale ACK validation (generation check)
    if ((tx->generation & 0xFFFFFF) != (generation & 0xFFFFFF)) {
        return;
    }

    int state = atomic_get(&tx->state);
    if (state != BH_CAP_TX_PUBLISHED && state != BH_CAP_TX_ABORTED) {
        return;
    }

    // Check if responder was part of target_mask
    if ((tx->target_mask & (1U << responder_core)) == 0) {
        return;
    }

    // Duplicate ACK check (idempotency)
    if ((tx->ack_mask & (1U << responder_core)) != 0) {
        return;
    }

    tx->ack_mask |= (1U << responder_core);
    if (status != 0 && tx->result == 0) {
        tx->result = (int32_t)status;
    }

    __asm__ volatile("" : : : "memory");
}

void cap_handle_delegate_req(uint64_t payload, uint32_t source_core) {
    cap_handle_tx_req(payload, source_core);
}

void cap_handle_delegate_ack(uint64_t payload) {
    cap_handle_tx_ack(payload);
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

static kstatus_t cap_validate_scope_internal(const capability_table_t *table, uint64_t requester_pid) {
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
            if (!bh_cap_generation_matches(e->generation, handle_gen)) {
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
