# ADR 008: Distributed VM Monitor and VM Spaces

## Status
Accepted

## Context
The current Virtual Memory Management (VMM) model (`kernel/src/mm/vmm.c` and `address_space_t`) assumes a monolithic, single-core perspective of address spaces. This design tightly couples the logical address space description with the physical page table programming (via `mmu_ops`).

As Bharat-OS evolves toward a distributed multikernel architecture, this monolithic approach presents several critical problems:
1. **Lack of Distributed Coherence:** There is no robust mechanism for cross-core TLB shootdowns, address-space migration, or remote invalidation.
2. **Hidden Assumptions:** Page tables are treated as the definitive source of truth, rather than a cached realization of a higher-level logical state.
3. **Profile Homogeneity:** It forces a single fake paging model onto all profiles, neglecting the realities of MPU/PMP-only systems (hard real-time) and systems with DMA isolation requirements (IOMMU).
4. **RT Constraints Ignored:** Timing constraints (hard vs. soft real-time) and memory pinning are not explicitly modeled in the core VM objects.

## Decision
We will transition to a **3-plane Distributed VM Architecture**:

1. **Plane A — Canonical Distributed VM Model (`vm_space_t`)**: This plane is the absolute source of truth. It tracks logical address spaces, mappings, capabilities, sharing rules, and generations. It is profile-aware (MMU, MPU, DMA-isolated) and timing-aware (Best Effort, Soft RT, Firm RT, Hard RT).
2. **Plane B — URPC Monitor Protocol (`mon_vm_proto.h`)**: The distributed control plane. It handles map/unmap propagation, remote invalidation, realization requests, activation hints for thread migration, and strict ACK flows for revocation.
3. **Plane C — Local Hardware Realization (`arch_vm_ops_t`)**: The silicon-facing data plane. It performs actual page-table writes, control register activation (CR3/TTBR/satp), local TLB invalidation, and MPU/PMP region programming on a per-core basis.

### Ownership and Coherence
- **Home Monitor Authority:** Each `vm_space_t` has a "home monitor" responsible for mutating the canonical mapping state, validating capabilities, and incrementing the generation counter.
- **Generation-Based Coherence:** Every mapping change increments the generation. The home monitor dispatches URPC messages to relevant cores.
- **Strict vs. Lazy Synchronization:** Unmaps, revocations, and permission downgrades use **strict mode** (requiring ACKs from all active/realized cores before returning). New mappings use **lazy mode** (canonical state updates immediately, cores realize it upon activation).

### Migration Strategy
To avoid a massive, destabilizing rewrite, we will build the new system alongside the legacy `vmm.c` implementation.
- The new VM subsystem (`vm_space.c`, `vm_mapping.c`, etc.) will be the core truth.
- The legacy VMM API will become an adapter/shim that translates old calls into the new structures.
- We will target `x86_64` as the initial proving ground (Phase 2), followed by ARM64 and RISC-V.

## Consequences
- **Positive:** True distributed memory consistency across cores. Explicit support for RT constraints and MPU/PMP systems without fake paging logic. Clean separation of policy (Monitor/VM Core) from mechanism (Arch MMU).
- **Negative/Cost:** Increased structural complexity. The transition period requires maintaining two VM APIs (legacy and new) and a compatibility shim. Context switching must now check realization generations.
