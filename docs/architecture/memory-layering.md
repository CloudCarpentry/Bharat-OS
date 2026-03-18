# Memory Boundary Audit

## Files by Layer

### PMM
`kernel/src/mm/pmm/pmm.c`
`kernel/src/mm/pmm/early_alloc.c`
`kernel/src/mm/pmm/kmem_aligned.c`
`kernel/src/mm/pmm/numa.c`
`kernel/src/mm/pmm/numa_policy.c`
`kernel/src/mm/pmm/pt_pool.c`
`kernel/src/mm/pmm/slab.c`

### VM Objects
`kernel/src/mm/vm/objects/vm_object.c`

### ASpace
`kernel/src/mm/vm/aspace/aspace.c`
`kernel/src/mm/vm/aspace/vm_space.c`

### Fault
`kernel/src/mm/vm/fault/fault.c`

### VMM / Legacy
`kernel/src/mm/vmm_legacy/vmm.c`
`kernel/src/mm/vmm_legacy/vm_compat_vmm.c`
`kernel/src/mm/vmm_legacy/vm_mapping.c`
`kernel/src/mm/vmm_legacy/mm_local.c`
`kernel/src/mm/vmm_legacy/mm_remote.c`
`kernel/src/mm/zswap.c`

### DMA / IOMMU
`kernel/src/mm/dma/dma.c`

## Removed Forbidden Dependencies / Architecture Leakage
1. `kernel/src/mm/vmm_legacy/vmm.c` direct access to `active_mmu` and `arch_mmu_init` were successfully replaced by the architecture-neutral `hal_pt` and `hal_tlb` contracts.
2. `kernel/src/mm/vmm_legacy/vmm.c` direct manipulation of `hal_tlb_flush` replaced with the `active_hal_tlb` neutral contract.
3. `kernel/src/mm/pmm/numa.c` legacy fallback usage of `hal_vmm_get_mapping` and `hal_vmm_update_mapping` removed and migrated to `active_hal_pt->query_page` and proper map/unmap routines.

## Strict Dependency Rules
- PMM layer has strictly zero knowledge of VM policy.
- VM Objects layer leverages PMM but remains isolated from architecture details.
- ASpace leverages the architecture-neutral HAL PT mappings.
- No DMA/IOMMU assumptions leak backwards into VM components.

## Remaining Build Issues
- The local emulator toolchain defaults to `/usr/bin/gcc` acting as a driver for `clang`, and lacks the `lld` linker. This is completely expected and purely an environment issue.
