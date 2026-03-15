#include "../../../include/mm/arch_vm.h"
#include "../../../include/hal/hal.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/mm.h"
#include <stddef.h>
#include <stdint.h>

// Mock definitions for the underlying HW
extern void write_cr3(phys_addr_t cr3);
extern phys_addr_t read_cr3(void);
extern void invlpg(uintptr_t addr);

static int x86_64_kernel_init(void) {
    // Bootstrap the global higher-half mapping
    return 0;
}

static int x86_64_space_init(vm_space_t *space, vm_core_state_t *local) {
    if (!space || !local) return -1;

    // Allocate PML4 root table
    // For MVP, just return a mocked value or reuse basic allocator
    local->root_pa = 0x1000; // Mock PA
    local->state = VM_REALIZE_VALID;
    return 0;
}

static void x86_64_space_destroy(vm_space_t *space, vm_core_state_t *local) {
    if (!space || !local) return;
    // Walk PML4, free PTs, free root
    local->root_pa = 0;
    local->state = VM_REALIZE_NONE;
}

static int x86_64_map(vm_space_t *space, vm_core_state_t *local,
                      uintptr_t va, uintptr_t pa, size_t len,
                      uint64_t prot, uint64_t mem_type, uint64_t flags) {
    (void)space; (void)local; (void)va; (void)pa; (void)len; (void)prot; (void)mem_type; (void)flags;
    // Local PT write using PML4 -> PDPT -> PD -> PT logic
    return 0;
}

static int x86_64_unmap(vm_space_t *space, vm_core_state_t *local,
                        uintptr_t va, size_t len) {
    (void)space; (void)local; (void)va; (void)len;
    // Remove local PT entries
    // invlpg(va);
    return 0;
}

static int x86_64_protect(vm_space_t *space, vm_core_state_t *local,
                          uintptr_t va, size_t len,
                          uint64_t prot, uint64_t mem_type) {
    (void)space; (void)local; (void)va; (void)len; (void)prot; (void)mem_type;
    // Modify existing local PT permissions
    // invlpg(va);
    return 0;
}

static int x86_64_activate(vm_space_t *space, vm_core_state_t *local) {
    if (!space || !local) return -1;

    // In actual implementation: write_cr3(local->root_pa);
    return 0;
}

static int x86_64_invalidate_range(vm_space_t *space, vm_core_state_t *local,
                                   uintptr_t va, size_t len, uint64_t flags) {
    (void)space; (void)local; (void)va; (void)len; (void)flags;
    // Execute multiple invlpg
    return 0;
}

static int x86_64_invalidate_all(vm_space_t *space, vm_core_state_t *local) {
    (void)space; (void)local;
    // Reload CR3
    // write_cr3(read_cr3());
    return 0;
}

static int x86_64_fault_decode(arch_trap_frame_t *tf, vm_fault_info_t *out) {
    (void)tf; (void)out;
    // Read CR2
    // Decode error code
    return 0;
}

const arch_vm_ops_t x86_64_arch_vm_ops = {
    .kernel_init = x86_64_kernel_init,
    .space_init = x86_64_space_init,
    .space_destroy = x86_64_space_destroy,
    .map = x86_64_map,
    .unmap = x86_64_unmap,
    .protect = x86_64_protect,
    .activate = x86_64_activate,
    .invalidate_range = x86_64_invalidate_range,
    .invalidate_all = x86_64_invalidate_all,
    .fault_decode = x86_64_fault_decode
};
