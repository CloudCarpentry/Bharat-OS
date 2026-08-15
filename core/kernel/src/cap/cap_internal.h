#ifndef BHARAT_CAP_INTERNAL_H
#define BHARAT_CAP_INTERNAL_H

#include "capability.h"
#include "bharat_config.h"
#include "cap_policy.h"
#include <bharat/cpu_local.h>
#include "kernel_safety.h"
#include "bharat/urpc.h"
#include "hal/hal.h"
#include "hal/hal_timer.h"
#include "lib/base/string.h"
#include "atomic.h"
#include "slab.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <bharat/cap/cap_validate.h>

#define BH_CAP_CSPACES_PER_CORE 64U
#define BH_CAP_CSPACE_SLOT_BITS 6U
#define BH_CAP_CSPACE_OWNER_BITS 5U
#define BH_CAP_CSPACE_SLOT_MASK ((UINT32_C(1) << BH_CAP_CSPACE_SLOT_BITS) - 1U)
#define BH_CAP_CSPACE_OWNER_SHIFT BH_CAP_CSPACE_SLOT_BITS
#define BH_CAP_CSPACE_GENERATION_SHIFT \
    (BH_CAP_CSPACE_SLOT_BITS + BH_CAP_CSPACE_OWNER_BITS)

#define BH_CAP_LOCATOR_NULL_CORE UINT16_MAX
#define BH_CAP_LOCATOR_NULL_SLOT UINT16_MAX

typedef struct {
    capability_table_t *table;
    uint32_t generation;
    bool uses_bootstrap_storage;
} bh_cap_cspace_registry_entry_t;

extern bh_cap_cspace_registry_entry_t g_cap_cspace_registry[MAX_CPUS][BH_CAP_CSPACES_PER_CORE];
extern capability_table_t g_cap_bootstrap_cspaces[MAX_CPUS];
extern bool g_cap_bootstrap_cspaces_used[MAX_CPUS];

#define BH_CAP_TX_PER_CORE 16U

typedef enum {
    BH_CAP_TX_FREE = 0,
    BH_CAP_TX_PREPARED = 1,
    BH_CAP_TX_PUBLISHED = 2,
    BH_CAP_TX_COMMITTED = 3,
    BH_CAP_TX_ABORTED = 4,
} bh_cap_tx_state_t;

typedef enum {
    BH_CAP_TX_OP_NONE = 0,
    BH_CAP_TX_OP_DELEGATE = 1,
    BH_CAP_TX_OP_REVOKE = 2,
    BH_CAP_TX_OP_ROLLBACK = 3,
} bh_cap_tx_op_t;

typedef struct {
    atomic_t state;                  // bh_cap_tx_state_t
    uint32_t generation;             // monotonically increasing per slot
    uint32_t op;                     // bh_cap_tx_op_t
    bh_cap_locator_t src;            // source capability locator
    bh_cap_locator_t dst;            // destination capability locator
    bh_cap_locator_t target;         // target locator for revoke
    uint64_t revocation_epoch;
    uint32_t type;                   // cap_type_t
    cap_rights_mask_t requested_rights;
    uint64_t object_ref;
    uint32_t flags;
    uint32_t owner_core;
    cap_instance_id_t instance_id;
    bh_cap_locator_t src_first_child;
    uint32_t target_mask;            // bitmask of cores expected to acknowledge
    uint32_t ack_mask;               // bitmask of cores that acknowledged
    int32_t result;                  // transaction result / error code

    // Remote allocation results populated by destination core:
    uint32_t remote_cap_id;
    uint32_t remote_dst_slot;
    uint32_t remote_dst_gen;
} bh_cap_tx_entry_t;

extern bh_cap_tx_entry_t g_cap_tx_table[MAX_CPUS][BH_CAP_TX_PER_CORE];
extern spinlock_t g_cap_tx_lock[MAX_CPUS];

static inline uint64_t cap_tx_pack_req(uint8_t origin_core, uint8_t slot, uint32_t generation, uint8_t op) {
    return ((uint64_t)origin_core << 48) |
           ((uint64_t)slot << 40) |
           (((uint64_t)generation & 0xFFFFFFFFULL) << 8) |
           ((uint64_t)op & 0xFFULL);
}

static inline void cap_tx_unpack_req(uint64_t payload, uint8_t *origin_core, uint8_t *slot, uint32_t *generation, uint8_t *op) {
    if (origin_core) *origin_core = (uint8_t)((payload >> 48) & 0xFF);
    if (slot) *slot = (uint8_t)((payload >> 40) & 0xFF);
    if (generation) *generation = (uint32_t)((payload >> 8) & 0xFFFFFFFFULL);
    if (op) *op = (uint8_t)(payload & 0xFF);
}

static inline uint64_t cap_tx_pack_ack(uint8_t origin_core, uint8_t slot, uint32_t generation, uint8_t responder_core, int8_t status) {
    return ((uint64_t)origin_core << 48) |
           ((uint64_t)slot << 40) |
           (((uint64_t)generation & 0xFFFFFFULL) << 16) |
           ((uint64_t)responder_core << 8) |
           ((uint64_t)(uint8_t)status);
}

static inline void cap_tx_unpack_ack(uint64_t payload, uint8_t *origin_core, uint8_t *slot, uint32_t *generation, uint8_t *responder_core, int8_t *status) {
    if (origin_core) *origin_core = (uint8_t)((payload >> 48) & 0xFF);
    if (slot) *slot = (uint8_t)((payload >> 40) & 0xFF);
    if (generation) *generation = (uint32_t)((payload >> 16) & 0xFFFFFFULL);
    if (responder_core) *responder_core = (uint8_t)((payload >> 8) & 0xFF);
    if (status) *status = (int8_t)(payload & 0xFF);
}

bh_cap_tx_entry_t *cap_tx_alloc(uint32_t origin_core, uint32_t op, uint8_t *out_slot);
void cap_tx_release(uint32_t origin_core, uint8_t slot);
void cap_handle_tx_req(uint64_t payload, uint32_t source_core);
void cap_handle_tx_ack(uint64_t payload);

static inline bh_cap_locator_t cap_locator_null(void) {
    return (bh_cap_locator_t){
        .cspace_id = 0U,
        .owner_core = BH_CAP_LOCATOR_NULL_CORE,
        .slot = BH_CAP_LOCATOR_NULL_SLOT,
        .generation = 0U,
        .revocation_epoch = 0U,
    };
}

static inline bool cap_locator_is_null(const bh_cap_locator_t *locator) {
    return locator->cspace_id == 0U;
}

static inline uint32_t cap_cspace_id_make(uint32_t owner_core, uint32_t slot,
                                          uint32_t generation) {
    return (generation << BH_CAP_CSPACE_GENERATION_SHIFT) |
           (owner_core << BH_CAP_CSPACE_OWNER_SHIFT) | (slot + 1U);
}

static inline bh_cap_locator_t cap_locator_make(const capability_table_t *table,
                                                uint32_t slot,
                                                uint32_t generation,
                                                uint32_t revocation_epoch) {
    if (table == NULL || slot >= BHARAT_ARRAY_SIZE(table->entries) ||
        slot > UINT16_MAX) {
        return cap_locator_null();
    }

    return (bh_cap_locator_t){
        .cspace_id = table->cspace_id,
        .owner_core = table->owner_core,
        .slot = (uint16_t)slot,
        .generation = generation,
        .revocation_epoch = revocation_epoch,
    };
}

static inline capability_table_t *cap_locator_resolve_table(const bh_cap_locator_t *locator) {
    if (locator == NULL || cap_locator_is_null(locator) ||
        locator->owner_core >= MAX_CPUS) {
        return NULL;
    }

    uint32_t encoded_slot = locator->cspace_id & BH_CAP_CSPACE_SLOT_MASK;
    if (encoded_slot == 0U || encoded_slot > BH_CAP_CSPACES_PER_CORE) {
        return NULL;
    }

    uint32_t slot = encoded_slot - 1U;
    bh_cap_cspace_registry_entry_t *registered =
        &g_cap_cspace_registry[locator->owner_core][slot];
    capability_table_t *table = registered->table;
    if (table == NULL ||
        table->cspace_id != locator->cspace_id ||
        table->owner_core != locator->owner_core ||
        table->registry_slot != slot) {
        return NULL;
    }
    return table;
}

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

static inline int cap_rights_valid(cap_type_t type, uint64_t rights) {
    return cap_transfer_rights_valid(type, rights);
}

void cap_lock_tables_sorted(capability_table_t** tables, size_t count);
void cap_unlock_tables_sorted(capability_table_t** tables, size_t count);

bharat_cap_status_t kernel_cap_authority_resolver(
    bharat_cap_handle_t handle,
    bharat_cap_object_type_t expected_object_type,
    uint64_t expected_object_id,
    uint64_t required_rights,
    const bharat_cap_scope_t *required_scope,
    bharat_cap_validation_result_t *out_result);

#endif /* BHARAT_CAP_INTERNAL_H */
