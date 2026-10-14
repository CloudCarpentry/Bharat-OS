---
name: bharat-os-memory-model
description: Apply when working on VM, PMM, MMU, MMU-Lite, MPU, PMP, DMA, HMEM, address spaces, page tables, or memory backends.
---

# Bharat-OS Memory Model Skill

## Tri-Model Memory Architecture

Bharat-OS targets diverse hardware profiles using three distinct protection models:

1. **`MMU_FULL`**:
   - Paged virtual memory with hierarchical page tables, page faults, demand paging, COW, and TLB shootdown across cores.
   - Used in desktop, server, mobile profiles (x86_64, aarch64, riscv64).
2. **`MMU_LITE`**:
   - Segmented/windowed protection without page tables; base/limit address isolation.
   - Low-latency embedded and lightweight systems (arm32, riscv32 mmu_lite profiles).
3. **`MPU/PMP`**:
   - Region-based hardware protection registers (ARM Cortex-M MPU, RISC-V PMP).
   - Zero-virtualization deterministic RTOS profiles without address translation.

## Architectural Invariants for Memory

1. **Backend Neutrality**:
   - Core kernel memory mechanisms must operate through the canonical abstraction: `aspace` (address space) -> `region` (virtual memory area) -> `vm_object` (backing physical memory or anonymous frame source).
   - Never hardcode MMU page table manipulation into general kernel mechanisms. Route through HAL/architecture memory operations table.
2. **Fail Closed**:
   - If a requested memory configuration or mapping type is unsupported by the current target's memory model (e.g. demanding demand-paging on MPU), the operation MUST return `BH_STATUS_ERR_UNSUPPORTED` immediately. Never silently downgrade security protections.
3. **No Recursive Memory Operations**:
   - Paging, fault-handling, and allocation routines must never trigger recursive allocations or nested faults that could blow the kernel stack.
   - Enforced by `python3 tools/lint/check_no_recursive_memops.py`.
4. **Physical Memory Management (PMM)**:
   - Core-local caching via `pmm_pcache` (magazines/slabs) avoids global lock contention.
   - Remote page frees must use bounded remote-free queues or messages, never direct remote lock acquisition.
   - HMEM/NUMA nodes must be respected during allocation passes.
5. **Fault-Safe Usercopy**:
   - **Never dereference user-provided pointers directly in the kernel.**
   - All data transfer between userland and kernel must occur through explicit, fault-isolated usercopy functions (`bh_usercopy_from_user()`, `bh_usercopy_to_user()`) that catch page faults, unmapped regions, and invalid address ranges safely.
6. **Address Space Lifecycle**:
   - Modifying an address space requires holding that aspace's lock.
   - Never hold an address space lock during cross-core operations (e.g. TLB shootdown). Release or stage the invalidation mask before waiting.

## Verification Checklist

- [ ] Check memory operations with `python3 tools/lint/check_no_recursive_memops.py`.
- [ ] Verify both 32-bit and 64-bit alignment and overflow checks before arithmetic on sizes/offsets.
- [ ] Test on MMU-Lite target: `arm32_mmu_lite_headless` and `riscv32_mmu_lite_headless`.
- [ ] Test on full MMU target: `x86_64_desktop_headless`.
- [ ] Confirm usercopy handles illegal addresses, unmapped pages, and cross-boundary buffers without panicking the kernel.
