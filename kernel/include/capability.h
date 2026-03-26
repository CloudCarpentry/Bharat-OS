#ifndef BHARAT_CAPABILITY_H
#define BHARAT_CAPABILITY_H

#include <stdint.h>

#include "sched/sched.h"
#include "spinlock.h"

typedef enum {
    CAP_TYPE_NONE = 0,
    CAP_TYPE_ENDPOINT = 1,
    CAP_TYPE_MEMORY = 2,
    CAP_TYPE_SCHED = 3,
    CAP_TYPE_PROCESS = 4,
    CAP_TYPE_CRYPTO_ENDPOINT = 5,
    CAP_TYPE_CRYPTO_DEVICE = 6,
    CAP_TYPE_CRYPTO_KEY = 7,
    CAP_TYPE_RNG = 8,
    CAP_TYPE_SEALER = 9,
    CAP_TYPE_NETDEV = 10,
    CAP_TYPE_NET_QUEUE = 11,
    CAP_TYPE_NET_BUFFER = 12,
    CAP_TYPE_IMPORTED_PROXY = 13, // Remote/delegated capability on wire

    // Accelerator and DMA types
    CAP_TYPE_ACCEL_DEVICE = 14,
    CAP_TYPE_ACCEL_QUEUE = 15,
    CAP_TYPE_ACCEL_BUFFER = 16,
    CAP_TYPE_ACCEL_TELEMETRY = 17,
    CAP_TYPE_ACCEL_ADMIN = 18,
    CAP_TYPE_DMA_DOMAIN = 19,
    CAP_TYPE_DMA_GRANT = 20,
} cap_type_t;

typedef enum {
    CAP_RIGHT_NONE                 = 0,
    CAP_RIGHT_SEND                 = (1U << 0),
    CAP_RIGHT_RECEIVE              = (1U << 1),
    CAP_RIGHT_MAP                  = (1U << 2),
    CAP_RIGHT_UNMAP                = (1U << 3),
    CAP_RIGHT_SCHEDULE             = (1U << 4),
    CAP_RIGHT_DELEGATE             = (1U << 5),
    CAP_RIGHT_CRYPT_USE            = (1U << 6),
    CAP_RIGHT_CRYPT_DERIVE         = (1U << 7),
    CAP_RIGHT_CRYPT_SIGN           = (1U << 8),
    CAP_RIGHT_CRYPT_DECRYPT        = (1U << 9),
    CAP_RIGHT_CRYPT_EXPORT_WRAPPED = (1U << 10),
    CAP_RIGHT_CRYPT_ADMIN          = (1U << 11),

    // Communication and RPC capability bits
    // NOTE: These are vocabulary only and not currently enforced in the
    // scheduler, IPC send/recv, or URPC routing paths yet.
    CAP_RIGHT_IPC_SEND             = (1U << 12),
    CAP_RIGHT_IPC_RECEIVE          = (1U << 13),
    CAP_RIGHT_URPC_CALL            = (1U << 14),
    CAP_RIGHT_URPC_REPLY           = (1U << 15),

    // Accelerator and DMA rights
    CAP_RIGHT_ENQUEUE              = (1U << 16),
    CAP_RIGHT_CANCEL               = (1U << 17),
    CAP_RIGHT_QUERY                = (1U << 18),
    CAP_RIGHT_BIND                 = (1U << 19),
    CAP_RIGHT_SHARE                = (1U << 20),
    CAP_RIGHT_SYNC_CPU             = (1U << 21),
    CAP_RIGHT_SYNC_DEV             = (1U << 22),
    CAP_RIGHT_READ_STATS           = (1U << 23),
    CAP_RIGHT_READ_FAULTS          = (1U << 24),
    CAP_RIGHT_RESET                = (1U << 25),
    CAP_RIGHT_PARTITION            = (1U << 26),
    CAP_RIGHT_FW_LOAD              = (1U << 27),
    CAP_RIGHT_DERIVE               = (1U << 28),
    CAP_RIGHT_REVOKE               = (1U << 29),
    CAP_RIGHT_READ                 = (1U << 30),
    CAP_RIGHT_WRITE                = (1U << 31),
    CAP_RIGHT_EXECUTE              = (1ULL << 32),

    // Synthetic rights combo
    CAP_RIGHT_ALL                  = 0xFFFFFFFF,
} cap_rights_t;

typedef struct capability_entry {
    uint8_t in_use;
    uint32_t id;
    cap_type_t type;
    uint32_t rights;
    uint32_t flags;
    uint64_t object_ref;

} capability_entry_old_t;

typedef struct __attribute__((aligned(16))) {
    struct capability_table* table;
    uint32_t slot;
    uint32_t generation;
} cap_handle_t;

typedef struct cap_instance_id {
    uint32_t origin_core;       // The core that authoritatively owns the object
    uint64_t object_id;         // The unique ID of the underlying kernel object
    uint32_t slot_gen;          // Generation number of the local CNode slot
    uint32_t rights_digest;     // Hash or bitmask of the granted rights
} cap_instance_id_t;

// Redefine capability_entry_t with handles to avoid use-after-free
typedef struct __attribute__((aligned(16))) capability_entry_new {
    uint8_t in_use;
    uint32_t id;
    cap_type_t type;
    uint32_t rights;
    uint32_t flags;
    uint64_t object_ref;

    cap_handle_t parent;       // Who delegated this to me?
    cap_handle_t first_child;  // Who did I delegate this to?
    cap_handle_t next_sibling; // Other capabilities delegated from the same parent

    uint32_t generation;

    // Memory/Frame Ownership
    uint32_t owner_core;

    uint8_t pad_mid[8]; // Align instance_id to 16-byte boundary (offset 96)

    // Distributed consistency fields
    cap_instance_id_t instance_id;
    uint64_t revocation_epoch;
} capability_entry_new_t;

// Replace old struct with new
#define capability_entry_t capability_entry_new_t

typedef struct __attribute__((aligned(64))) capability_table {
    capability_entry_t entries[64];
    uint32_t next_id;
    spinlock_t lock;
    uint32_t owner_pid;
    uint32_t numa_node;
} capability_table_t;

/*@
  ensures \result == \null || \valid(\result);
*/
capability_table_t* cap_table_create(void);

/*@
  requires proc != \null;
  requires \valid(proc);
  assigns proc->security_sandbox_ctx;
  ensures \result == 0 || \result == -1 || \result == -2;
*/
int cap_table_init_for_process(kprocess_t* proc);
void cap_table_destroy(capability_table_t* table);

/*@
  requires table != \null;
  requires \valid(table);
  requires out_cap_id == \null || \valid(out_cap_id);
  behavior with_out_cap_id:
    assumes out_cap_id != \null;
    assigns table->entries[0..63], table->next_id, *out_cap_id;
  behavior without_out_cap_id:
    assumes out_cap_id == \null;
    assigns table->entries[0..63], table->next_id;
  complete behaviors;
  disjoint behaviors;
  ensures \result == 0 || \result == -1 || \result == -2 || \result == -3;
*/
int cap_table_grant(capability_table_t* table,
                    cap_type_t type,
                    uint64_t object_ref,
                    uint32_t rights,
                    uint32_t* out_cap_id);

/*@
  requires table != \null;
  requires \valid_read(table);
  requires out_entry == \null || \valid(out_entry);
  behavior with_out_entry:
    assumes out_entry != \null;
    assigns *out_entry;
  behavior without_out_entry:
    assumes out_entry == \null;
    assigns \nothing;
  complete behaviors;
  disjoint behaviors;
  ensures \result == 0 || \result == -1 || \result == -2 || \result == -3 || \result == -4;
*/
int cap_table_lookup(const capability_table_t* table,
                     uint32_t cap_id,
                     cap_type_t required_type,
                     uint32_t required_rights,
                     capability_entry_t* out_entry);

/*@
  requires src != \null;
  requires dst != \null;
  requires \valid(src);
  requires \valid(dst);
  requires out_new_cap_id == \null || \valid(out_new_cap_id);
  behavior with_out_new_cap_id:
    assumes out_new_cap_id != \null;
    assigns dst->entries[0..63], dst->next_id, *out_new_cap_id;
  behavior without_out_new_cap_id:
    assumes out_new_cap_id == \null;
    assigns dst->entries[0..63], dst->next_id;
  complete behaviors;
  disjoint behaviors;
  ensures \result == 0 || \result == -1 || \result == -2 || \result == -3 || \result == -4 || \result == -5;
*/
int cap_table_delegate(capability_table_t* src,
                       capability_table_t* dst,
                       uint32_t cap_id,
                       uint32_t delegated_rights,
                       uint32_t* out_new_cap_id);

int cap_table_revoke(capability_table_t* table, uint32_t cap_id);

void cap_handle_delegate_req(uint64_t payload, uint32_t source_core);
void cap_handle_delegate_ack(uint64_t payload);
void cap_handle_revoke_req(uint64_t payload, uint32_t source_core);
void cap_handle_revoke_ack(uint64_t payload);

#endif // BHARAT_CAPABILITY_H
