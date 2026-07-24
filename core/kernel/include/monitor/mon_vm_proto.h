#ifndef BHARAT_MON_VM_PROTO_H
#define BHARAT_MON_VM_PROTO_H

#include <stdint.h>

// Message types
typedef enum {
    MON_VM_SPACE_CREATE = 0x1000,
    MON_VM_SPACE_DESTROY,

    MON_VM_MAP,
    MON_VM_UNMAP,
    MON_VM_PROTECT,
    MON_VM_REALIZE,
    MON_VM_SYNC_GENERATION,

    MON_VM_ACTIVATE_HINT,
    MON_VM_DEACTIVATE_HINT,
    MON_VM_PREPARE_RT,

    MON_VM_TLB_INVALIDATE_RANGE,
    MON_VM_TLB_INVALIDATE_ALL,

    MON_VM_FAULT_NOTIFY,
    MON_VM_FAULT_RESOLVE,

    MON_VM_ACK,
    MON_VM_NACK
} mon_vm_msg_type_t;

// Common Header for VM URPC messages
typedef struct mon_vm_hdr {
    uint32_t type;         // mon_vm_msg_type_t
    uint32_t flags;
    uint64_t txn_id;
    uint64_t space_id;
    uint64_t generation;
    uint32_t src_core;
    uint32_t dst_core;
    uint32_t reserved;
} mon_vm_hdr_t;

// Protocol Flags
#define MON_VM_F_STRICT_ACK        (1U << 0)
#define MON_VM_F_RT_CRITICAL       (1U << 1)
#define MON_VM_F_NO_LAZY_REALIZE   (1U << 2)
#define MON_VM_F_PREPARE_ONLY      (1U << 3)
#define MON_VM_F_REBUILD_FULL      (1U << 4)

// Message Bodies
typedef struct mon_vm_map_msg {
    mon_vm_hdr_t h;
    uintptr_t va_start;
    uintptr_t pa_start;
    uint64_t length;
    uint64_t prot;
    uint64_t mem_type;
    uint64_t map_flags;
    // For MVP, just pass a handle slot or ID across URPC.
    // Usually this requires a capability transfer via IPC.
    uint64_t backing_cap_slot;
} mon_vm_map_msg_t;

typedef struct mon_vm_inv_msg {
    mon_vm_hdr_t h;
    uintptr_t va_start;
    uint64_t length;
} mon_vm_inv_msg_t;

typedef struct mon_vm_ack_msg {
    mon_vm_hdr_t h;
    int32_t status;
    uint32_t realize_state;
} mon_vm_ack_msg_t;

#endif // BHARAT_MON_VM_PROTO_H
