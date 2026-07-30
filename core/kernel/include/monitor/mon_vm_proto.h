#ifndef BHARAT_MON_VM_PROTO_H
#define BHARAT_MON_VM_PROTO_H

#include <stdint.h>

// Protocol versions
#define MON_VM_PROTO_VERSION_MAJOR 1
#define MON_VM_PROTO_VERSION_MINOR 0

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

// Protocol Flags
#define MON_VM_F_STRICT_ACK        (1U << 0)
#define MON_VM_F_RT_CRITICAL       (1U << 1)
#define MON_VM_F_NO_LAZY_REALIZE   (1U << 2)
#define MON_VM_F_PREPARE_ONLY      (1U << 3)
#define MON_VM_F_REBUILD_FULL      (1U << 4)

// Transaction Handle
typedef struct {
    uint16_t origin_core;
    uint16_t slot;
    uint32_t generation;
} __attribute__((packed, aligned(4))) mon_vm_tx_handle_t;

// Common Header for VM URPC messages (exactly 56 bytes)
typedef struct {
    uint8_t version_major;
    uint8_t version_minor;
    uint16_t reserved_version;
    uint32_t type;         // mon_vm_msg_type_t
    uint32_t flags;
    uint32_t payload_len;  // At offset 12: pads to 16

    uint64_t space_id;            // At offset 16
    uint64_t expected_generation; // At offset 24
    uint64_t proposed_generation; // At offset 32

    uint32_t src_core;     // At offset 40
    uint32_t dst_core;     // At offset 44

    // Explicit transaction handle fields on the wire
    uint16_t tx_origin_core; // At offset 48
    uint16_t tx_slot;        // At offset 50
    uint32_t tx_generation;  // At offset 52
} __attribute__((packed, aligned(8))) mon_vm_hdr_t;

// Message Bodies
typedef struct {
    mon_vm_hdr_t h;
    uint64_t va_start;
    uint64_t pa_start;
    uint64_t length;
    uint64_t prot;
    uint64_t mem_type;
    uint64_t map_flags;
    uint64_t backing_cap_slot;
    uint64_t reserved_pad; // pad to 120
} __attribute__((packed, aligned(8))) mon_vm_map_msg_t;

typedef struct {
    mon_vm_hdr_t h;
    uint64_t va_start;
    uint64_t length;
    uint64_t prot;
    uint64_t mem_type;
    uint64_t reserved_pad; // pad to 96
} __attribute__((packed, aligned(8))) mon_vm_protect_msg_t;

typedef struct {
    mon_vm_hdr_t h;
    uint64_t va_start;
    uint64_t length;
    uint64_t reserved_pad; // pad to 80 (note: asserted to be 80 if we want, or we keep it 72 or 80. Let's make it 80 to align nicely on 8 bytes)
} __attribute__((packed, aligned(8))) mon_vm_unmap_msg_t;

typedef struct {
    mon_vm_hdr_t h;
    int32_t status;
    uint32_t realize_state;
} __attribute__((packed, aligned(8))) mon_vm_ack_msg_t;

// Fixed-width unified message envelope (exactly 128 bytes)
typedef union {
    mon_vm_hdr_t h;
    mon_vm_map_msg_t map;
    mon_vm_unmap_msg_t unmap;
    mon_vm_protect_msg_t protect;
    mon_vm_ack_msg_t ack;
    uint8_t raw[128];
} __attribute__((packed, aligned(8))) mon_vm_msg_t;

// Compile-time static assertions for size and alignments
_Static_assert(sizeof(mon_vm_tx_handle_t) == 8, "mon_vm_tx_handle_t must be 8 bytes");
_Static_assert(sizeof(mon_vm_hdr_t) == 56, "mon_vm_hdr_t must be 56 bytes");
_Static_assert(sizeof(mon_vm_map_msg_t) == 120, "mon_vm_map_msg_t must be 120 bytes");
_Static_assert(sizeof(mon_vm_protect_msg_t) == 96, "mon_vm_protect_msg_t must be 96 bytes");
_Static_assert(sizeof(mon_vm_unmap_msg_t) == 80, "mon_vm_unmap_msg_t must be 80 bytes");
_Static_assert(sizeof(mon_vm_ack_msg_t) == 64, "mon_vm_ack_msg_t must be 64 bytes");
_Static_assert(sizeof(mon_vm_msg_t) == 128, "mon_vm_msg_t must be exactly 128 bytes");

// Offsets checks
_Static_assert(__builtin_offsetof(mon_vm_hdr_t, space_id) == 16, "space_id offset must be 16");
_Static_assert(__builtin_offsetof(mon_vm_hdr_t, src_core) == 40, "src_core offset must be 40");
_Static_assert(__builtin_offsetof(mon_vm_hdr_t, tx_origin_core) == 48, "tx_origin_core offset must be 48");

#endif // BHARAT_MON_VM_PROTO_H
