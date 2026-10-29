#ifndef BHARAT_UAPI_SYSTEM_VMM_H
#define BHARAT_UAPI_SYSTEM_VMM_H

#include <stdint.h>
#include <stddef.h>

/* Object operations on BH_SYS_CAPABILITY_INVOKE, not new syscall numbers.
 * arg0 points to this exact versioned request. arg1 is reserved and must be 0.
 * No operation accepts physical addresses or a claimed caller identity.
 * Version 1 is reserved but currently returns UNSUPPORTED after authorization:
 * backing extent and lifetime pinning must be implemented before activation. */
#define BH_VM_ABI_VERSION 1U
#define BH_VM_OP_CREATE 1U
#define BH_VM_OP_DESTROY 2U
#define BH_VM_OP_MAP 3U
#define BH_VM_OP_UNMAP 4U
#define BH_VM_OP_PROTECT 5U
#define BH_VM_OP_QUERY 6U

#define BH_VM_PROT_READ UINT64_C(1)
#define BH_VM_PROT_WRITE UINT64_C(2)
#define BH_VM_PROT_EXEC UINT64_C(4)

typedef struct {
    uint32_t version;
    uint32_t struct_size;
    uint64_t address;
    uint64_t length;
    uint64_t backing_offset;
    uint64_t protection;
    uint32_t backing_cap;
    uint32_t profile;
    uint32_t timing_class;
    uint32_t reserved;
} bh_vm_request_t;

_Static_assert(sizeof(bh_vm_request_t) == 56, "VM request ABI size");
_Static_assert(offsetof(bh_vm_request_t, backing_cap) == 40, "VM backing cap offset");

#endif
