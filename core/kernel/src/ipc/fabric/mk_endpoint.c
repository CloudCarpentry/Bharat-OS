#include "ipc/mk_proto.h"
#include <hal/hal.h>

kstatus_t bh_mk_endpoint_bind(
    const bh_mk_endpoint_config_t *config,
    bh_mk_endpoint_handle_t *out_endpoint)
{
    if (!config || !out_endpoint || !config->handler_fn) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    for (uint32_t i = 0; i < BH_MK_MAX_ENDPOINTS; i++) {
        bh_mk_endpoint_entry_t *entry = &f->endpoints.entries[i];
        if (!entry->bound) {
            entry->handler_fn = config->handler_fn;
            entry->ctx = config->ctx;
            entry->message_class = config->message_class;
            entry->opcode = config->opcode;
            entry->bound = 1;
            entry->generation++;

            // Pack the 64-bit handle: core_id, core_generation, endpoint_generation, slot
            uint32_t core_gen = atomic_load_explicit(&f->generation, memory_order_relaxed);
            kstatus_t pack_status = bh_mk_handle_pack(core_id, core_gen, entry->generation, i, out_endpoint);
            if (pack_status != K_OK) {
                entry->bound = 0;
                return pack_status;
            }
            return K_OK;
        }
    }

    return K_ERR_NO_RESOURCES;
}

kstatus_t bh_mk_endpoint_unbind(bh_mk_endpoint_handle_t handle) {
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) {
        return K_ERR_BAD_STATE;
    }

    uint32_t h_core_id, h_core_gen, h_ep_gen, h_slot;
    kstatus_t unpack_status = bh_mk_handle_unpack(handle, &h_core_id, &h_core_gen, &h_ep_gen, &h_slot);
    if (unpack_status != K_OK) {
        return unpack_status;
    }

    if (h_core_id != core_id) {
        return K_ERR_WRONG_AFFINITY;
    }

    bh_mk_endpoint_entry_t *entry = &f->endpoints.entries[h_slot];
    if (!entry->bound || entry->generation != h_ep_gen) {
        return K_ERR_CAP_STALE;
    }

    entry->bound = 0;
    return K_OK;
}

kstatus_t bh_mk_endpoint_resolve(
    bh_mk_core_fabric_t *f,
    bh_mk_endpoint_handle_t handle,
    bh_mk_endpoint_entry_t **out_entry)
{
    uint32_t h_core_id, h_core_gen, h_ep_gen, h_slot;
    kstatus_t unpack_status = bh_mk_handle_unpack(handle, &h_core_id, &h_core_gen, &h_ep_gen, &h_slot);
    if (unpack_status != K_OK) {
        return unpack_status;
    }

    uint32_t core_id = hal_cpu_get_id();
    if (h_core_id != core_id) {
        f = bh_mk_get_core_fabric(h_core_id);
        if (!f) return K_ERR_BAD_STATE;
    }

    bh_mk_endpoint_entry_t *entry = &f->endpoints.entries[h_slot];
    if (!entry->bound || entry->generation != h_ep_gen) {
        return K_ERR_CAP_STALE;
    }

    if (out_entry) {
        *out_entry = entry;
    }
    return K_OK;
}
