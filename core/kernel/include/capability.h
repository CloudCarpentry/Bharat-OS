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
    CAP_TYPE_THREAD = 21,
} cap_type_t;

typedef uint64_t cap_rights_mask_t;

// Keep enum for symbolic names/indices, but avoid relying on its underlying size
typedef enum {
    CAP_RIGHT_BIT_ENDPOINT_SEND        = 0,
    CAP_RIGHT_BIT_ENDPOINT_RECEIVE     = 1,
    CAP_RIGHT_BIT_MEMORY_MAP           = 2,
    CAP_RIGHT_BIT_MEMORY_UNMAP         = 3,
    CAP_RIGHT_BIT_MEMORY_SHARE         = 4,
    CAP_RIGHT_BIT_DMA_MAP              = 5,
    CAP_RIGHT_BIT_SCHEDULE             = 6,
    CAP_RIGHT_BIT_DELEGATE             = 7,
    CAP_RIGHT_BIT_CRYPT_USE            = 8,
    CAP_RIGHT_BIT_CRYPT_DERIVE         = 9,
    CAP_RIGHT_BIT_CRYPT_SIGN           = 10,
    CAP_RIGHT_BIT_CRYPT_DECRYPT        = 11,
    CAP_RIGHT_BIT_CRYPT_EXPORT_WRAPPED = 12,
    CAP_RIGHT_BIT_CRYPT_ADMIN          = 13,

    // Communication and RPC capability bits
    CAP_RIGHT_BIT_IPC_SEND             = 14,
    CAP_RIGHT_BIT_IPC_RECEIVE          = 15,
    CAP_RIGHT_BIT_URPC_CALL            = 16,
    CAP_RIGHT_BIT_URPC_REPLY           = 17,

    // Accelerator and DMA rights
    CAP_RIGHT_BIT_ENQUEUE              = 18,
    CAP_RIGHT_BIT_CANCEL               = 19,
    CAP_RIGHT_BIT_QUERY                = 20,
    CAP_RIGHT_BIT_BIND                 = 21,
    CAP_RIGHT_BIT_SYNC_CPU             = 22,
    CAP_RIGHT_BIT_SYNC_DEV             = 23,
    CAP_RIGHT_BIT_READ_STATS           = 24,
    CAP_RIGHT_BIT_READ_FAULTS          = 25,
    CAP_RIGHT_BIT_RESET                = 26,
    CAP_RIGHT_BIT_PARTITION            = 27,
    CAP_RIGHT_BIT_FW_LOAD              = 28,
    CAP_RIGHT_BIT_DERIVE               = 29,
    CAP_RIGHT_BIT_REVOKE               = 30,
    CAP_RIGHT_BIT_READ                 = 31,
    CAP_RIGHT_BIT_WRITE                = 32,
    CAP_RIGHT_BIT_EXECUTE              = 33,
    CAP_RIGHT_BIT_VMM_MANAGE           = 34,
    CAP_RIGHT_BIT_FAULT_DOMAIN_MANAGE  = 35,
    CAP_RIGHT_BIT_PROCESS_MANAGE       = 36,
    CAP_RIGHT_BIT_RESOURCE_ALLOC       = 37,
} cap_rights_t;

// Standardize capability right mask macros on uint64_t
#define CAP_RIGHT_NONE                 UINT64_C(0)
#define CAP_RIGHT_ENDPOINT_SEND        (UINT64_C(1) << CAP_RIGHT_BIT_ENDPOINT_SEND)
#define CAP_RIGHT_ENDPOINT_RECEIVE     (UINT64_C(1) << CAP_RIGHT_BIT_ENDPOINT_RECEIVE)
#define CAP_RIGHT_MEMORY_MAP           (UINT64_C(1) << CAP_RIGHT_BIT_MEMORY_MAP)
#define CAP_RIGHT_MEMORY_UNMAP         (UINT64_C(1) << CAP_RIGHT_BIT_MEMORY_UNMAP)
#define CAP_RIGHT_MEMORY_SHARE         (UINT64_C(1) << CAP_RIGHT_BIT_MEMORY_SHARE)
#define CAP_RIGHT_DMA_MAP              (UINT64_C(1) << CAP_RIGHT_BIT_DMA_MAP)
#define CAP_RIGHT_SCHEDULE             (UINT64_C(1) << CAP_RIGHT_BIT_SCHEDULE)
#define CAP_RIGHT_DELEGATE             (UINT64_C(1) << CAP_RIGHT_BIT_DELEGATE)
#define CAP_RIGHT_CRYPT_USE            (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_USE)
#define CAP_RIGHT_CRYPT_DERIVE         (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_DERIVE)
#define CAP_RIGHT_CRYPT_SIGN           (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_SIGN)
#define CAP_RIGHT_CRYPT_DECRYPT        (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_DECRYPT)
#define CAP_RIGHT_CRYPT_EXPORT_WRAPPED (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_EXPORT_WRAPPED)
#define CAP_RIGHT_CRYPT_ADMIN          (UINT64_C(1) << CAP_RIGHT_BIT_CRYPT_ADMIN)

#define CAP_RIGHT_IPC_SEND             (UINT64_C(1) << CAP_RIGHT_BIT_IPC_SEND)
#define CAP_RIGHT_IPC_RECEIVE          (UINT64_C(1) << CAP_RIGHT_BIT_IPC_RECEIVE)
#define CAP_RIGHT_URPC_CALL            (UINT64_C(1) << CAP_RIGHT_BIT_URPC_CALL)
#define CAP_RIGHT_URPC_REPLY           (UINT64_C(1) << CAP_RIGHT_BIT_URPC_REPLY)

#define CAP_RIGHT_ENQUEUE              (UINT64_C(1) << CAP_RIGHT_BIT_ENQUEUE)
#define CAP_RIGHT_CANCEL               (UINT64_C(1) << CAP_RIGHT_BIT_CANCEL)
#define CAP_RIGHT_QUERY                (UINT64_C(1) << CAP_RIGHT_BIT_QUERY)
#define CAP_RIGHT_BIND                 (UINT64_C(1) << CAP_RIGHT_BIT_BIND)
#define CAP_RIGHT_SYNC_CPU             (UINT64_C(1) << CAP_RIGHT_BIT_SYNC_CPU)
#define CAP_RIGHT_SYNC_DEV             (UINT64_C(1) << CAP_RIGHT_BIT_SYNC_DEV)
#define CAP_RIGHT_READ_STATS           (UINT64_C(1) << CAP_RIGHT_BIT_READ_STATS)
#define CAP_RIGHT_READ_FAULTS          (UINT64_C(1) << CAP_RIGHT_BIT_READ_FAULTS)
#define CAP_RIGHT_RESET                (UINT64_C(1) << CAP_RIGHT_BIT_RESET)
#define CAP_RIGHT_PARTITION            (UINT64_C(1) << CAP_RIGHT_BIT_PARTITION)
#define CAP_RIGHT_FW_LOAD              (UINT64_C(1) << CAP_RIGHT_BIT_FW_LOAD)
#define CAP_RIGHT_DERIVE               (UINT64_C(1) << CAP_RIGHT_BIT_DERIVE)
#define CAP_RIGHT_REVOKE               (UINT64_C(1) << CAP_RIGHT_BIT_REVOKE)
#define CAP_RIGHT_READ                 (UINT64_C(1) << CAP_RIGHT_BIT_READ)
#define CAP_RIGHT_WRITE                (UINT64_C(1) << CAP_RIGHT_BIT_WRITE)
#define CAP_RIGHT_EXECUTE              (UINT64_C(1) << CAP_RIGHT_BIT_EXECUTE)
#define CAP_RIGHT_VMM_MANAGE           (UINT64_C(1) << CAP_RIGHT_BIT_VMM_MANAGE)
#define CAP_RIGHT_FAULT_DOMAIN_MANAGE  (UINT64_C(1) << CAP_RIGHT_BIT_FAULT_DOMAIN_MANAGE)
#define CAP_RIGHT_PROCESS_MANAGE       (UINT64_C(1) << CAP_RIGHT_BIT_PROCESS_MANAGE)
#define CAP_RIGHT_RESOURCE_ALLOC       (UINT64_C(1) << CAP_RIGHT_BIT_RESOURCE_ALLOC)

#define CAP_RIGHT_ALL                  (~UINT64_C(0))

typedef struct capability_entry {
    uint8_t in_use;
    uint32_t id;
    cap_type_t type;
    cap_rights_mask_t rights;
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
    uint64_t rights_digest;     // Hash or bitmask of the granted rights
} cap_instance_id_t;

// Redefine capability_entry_t with handles to avoid use-after-free
typedef struct __attribute__((aligned(16))) capability_entry_new {
    uint8_t in_use;
    uint8_t state; // cap_state_t
    uint32_t id;
    cap_type_t type;
    cap_rights_mask_t rights;
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

typedef enum {
    CAP_STATE_FREE = 0,
    CAP_STATE_LIVE = 1,
    CAP_STATE_REVOKING = 2,
    CAP_STATE_REVOKED = 3,
} cap_state_t;

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
int cap_table_init_for_process(bh_process_t* proc);
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
                    cap_rights_mask_t rights,
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
                     cap_rights_mask_t required_rights,
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
                       cap_rights_mask_t delegated_rights,
                       uint32_t* out_new_cap_id);

int cap_table_revoke(capability_table_t* table, uint32_t cap_id);

void cap_handle_delegate_req(uint64_t payload, uint32_t source_core);
void cap_handle_delegate_ack(uint64_t payload);
void cap_handle_revoke_req(uint64_t payload, uint32_t source_core);
void cap_handle_revoke_ack(uint64_t payload);

#endif // BHARAT_CAPABILITY_H
