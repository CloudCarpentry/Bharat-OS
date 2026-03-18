#ifndef BHARAT_ARCH_VM_H
#define BHARAT_ARCH_VM_H

#include "vm_space.h"
#include "vm_fault.h"

// Forward declaration for trap frame (which depends on arch)
typedef struct arch_trap_frame arch_trap_frame_t;

// Standard MMU Operations
typedef struct arch_vm_ops {
    int  (*kernel_init)(void);

    int  (*space_init)(vm_space_t *space, vm_core_state_t *local);
    void (*space_destroy)(vm_space_t *space, vm_core_state_t *local);

    int  (*map)(vm_space_t *space, vm_core_state_t *local,
                uintptr_t va, uintptr_t pa, size_t len,
                uint64_t prot, uint64_t mem_type, uint64_t flags);

    int  (*unmap)(vm_space_t *space, vm_core_state_t *local,
                  uintptr_t va, size_t len);

    int  (*protect)(vm_space_t *space, vm_core_state_t *local,
                    uintptr_t va, size_t len,
                    uint64_t prot, uint64_t mem_type);

    int  (*activate)(vm_space_t *space, vm_core_state_t *local);

    int  (*invalidate_range)(vm_space_t *space, vm_core_state_t *local,
                             uintptr_t va, size_t len, uint64_t flags);
    int  (*invalidate_all)(vm_space_t *space, vm_core_state_t *local);

    int  (*fault_decode)(arch_trap_frame_t *tf, vm_fault_info_t *out);
} arch_vm_ops_t;

// MPU/PMP Operations (for RT/Low-end profiles)
typedef struct arch_mem_domain_ops {
    int  (*domain_init)(vm_space_t *space);
    int  (*domain_destroy)(vm_space_t *space);

    int  (*region_program)(vm_space_t *space,
                           uintptr_t base, size_t size,
                           uint64_t prot, uint64_t flags);

    int  (*region_remove)(vm_space_t *space, uintptr_t base, size_t size);
    int  (*domain_activate)(vm_space_t *space);
} arch_mem_domain_ops_t;

extern const arch_vm_ops_t* active_arch_vm_ops;

#endif // BHARAT_ARCH_VM_H
