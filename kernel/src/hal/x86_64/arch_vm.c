#include "../../../include/mm/arch_vm.h"
#include "../../../include/hal/hal.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/hal/x86_64/trap_frame_x86.h"
#include "../../../include/mm.h"
#include <stddef.h>
#include <stdint.h>

extern phys_addr_t vmm_get_kernel_root(void);

static int x86_64_kernel_init(void) {
    // Kernel higher-half mapping is established in boot.S / mmu_init.c
    // before this point. Nothing to do here.
    return 0;
}

static int x86_64_space_init(vm_space_t *space, vm_core_state_t *local) {
    if (!space || !local || !active_mmu) return -1;

    // Clone kernel page table so upper-half kernel mappings are shared
    phys_addr_t root = active_mmu->clone_kernel(vmm_get_kernel_root());
    if (!root) return -1;

    local->root_pa = root;
    local->core_id = hal_cpu_get_id();
    local->state = VM_REALIZE_VALID;
    return 0;
}

static void x86_64_space_destroy(vm_space_t *space, vm_core_state_t *local) {
    if (!space || !local || !active_mmu) return;

    if (local->root_pa) {
        active_mmu->destroy_table(local->root_pa);
        local->root_pa = 0;
    }
    local->state = VM_REALIZE_NONE;
}

static int x86_64_map(vm_space_t *space, vm_core_state_t *local,
                      uintptr_t va, uintptr_t pa, size_t len,
                      uint64_t prot, uint64_t mem_type, uint64_t flags) {
    (void)space; (void)mem_type; (void)flags;
    if (!active_mmu || !local) return -1;
    return active_mmu->map(local->root_pa, va, pa, len, (mmu_flags_t)prot);
}

static int x86_64_unmap(vm_space_t *space, vm_core_state_t *local,
                        uintptr_t va, size_t len) {
    (void)space;
    if (!active_mmu || !local) return -1;
    return active_mmu->unmap(local->root_pa, va, len, NULL);
}

static int x86_64_protect(vm_space_t *space, vm_core_state_t *local,
                          uintptr_t va, size_t len,
                          uint64_t prot, uint64_t mem_type) {
    (void)space; (void)mem_type;
    if (!active_mmu || !local) return -1;
    return active_mmu->protect(local->root_pa, va, len, (mmu_flags_t)prot);
}

static int x86_64_activate(vm_space_t *space, vm_core_state_t *local) {
    (void)space;
    if (!active_mmu || !local) return -1;

    active_mmu->activate(local->root_pa);
    if (active_mmu->tlb_flush_all) {
        active_mmu->tlb_flush_all();
    }
    return 0;
}

static int x86_64_invalidate_range(vm_space_t *space, vm_core_state_t *local,
                                   uintptr_t va, size_t len, uint64_t flags) {
    (void)space; (void)local; (void)flags;
    if (!active_mmu || !active_mmu->tlb_flush_page) return -1;

    size_t offset = 0;
    while (offset < len) {
        active_mmu->tlb_flush_page(va + offset);
        offset += PAGE_SIZE;
    }
    return 0;
}

static int x86_64_invalidate_all(vm_space_t *space, vm_core_state_t *local) {
    (void)space; (void)local;
    if (!active_mmu || !active_mmu->tlb_flush_all) return -1;

    active_mmu->tlb_flush_all();
    return 0;
}

static int x86_64_fault_decode(arch_trap_frame_t *tf, vm_fault_info_t *out) {
    if (!tf || !out) return -1;

    // Safe cast — trap_entry.S always builds a full x86_trap_frame_t on x86
    const x86_trap_frame_t *x86tf = (const x86_trap_frame_t *)tf;

    out->addr   = x86tf->cr2;
    uint64_t ec     = x86tf->error_code;

    bool is_present = (ec >> 0) & 1;   // 0 = not-present, 1 = protection
    out->is_write   = (ec >> 1) & 1;   // write fault
    out->is_user    = (ec >> 2) & 1;   // user mode
    out->is_exec    = (ec >> 4) & 1;   // NX / instruction fetch

    out->not_present = !is_present;
    out->permission_fault = is_present;

    out->arch_code = ec;

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
