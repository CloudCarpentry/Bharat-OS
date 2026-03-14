# Memory Gap Closure Plan (PMM → VM → MMU → Pager → DMA)

This document converts the current memory-management gap assessment into an implementation backlog that can be executed incrementally.

## 1) Current-state rewrite (authoritative)

## Implementation progress snapshot

- ✅ Phase 0 (API surface bootstrap) started: memory and HAL contract headers landed under `kernel/include/mm/` and `kernel/include/hal/` to provide stable interfaces for PMM/VM object/aspace/fault/DMA and page-table/TLB/IOMMU work.

Bharat-OS has a credible baseline for physical memory management (PMM/buddy allocator) and software-visible mapping bookkeeping (VMM registry). However, a production-grade, architecture-complete virtual memory stack is still in progress:

- hardware page-table lifecycle and permission correctness are not fully mature across all targets,
- SMP-safe TLB invalidation discipline is incomplete,
- demand paging, COW, and pager integration are partial/deferred,
- NUMA/topology policy is early,
- DMA/IOMMU-aware mapping lifecycle is not yet end-to-end.

## 2) Layered target architecture (must keep boundaries strict)

To avoid coupling and architecture leakage, implement and enforce these layers:

1. **PMM layer** (`pmm/`): frame discovery, zones, buddy/contiguous alloc, refcounts, pinned/reserved classes.
2. **VM Object layer** (`vm/objects/`): anonymous/file/shared/device/DMA objects + COW semantics.
3. **Address-space layer** (`vm/aspace/`): region reservations, overlap checks, attach/detach object mappings.
4. **Hardware page-table layer** (`hal/mmu/arch/*`): map/unmap/protect/query + page-table page lifecycle + TLB rules.
5. **Fault/Pager layer** (`vm/fault/`): lazy allocation, COW break, stack growth, page-in/out policy.
6. **DMA/IOMMU layer** (`dma/`, `iommu/`): pinning, IOVA domains, cache sync, device-visible mappings.

## 3) Profile guarantees (explicit, non-negotiable)

Define and enforce profile-specific guarantees in build/config and docs:

- `PROFILE_MMU_FULL`: full VM + demand paging + COW + shared memory + pager hooks.
- `PROFILE_MMU_LITE`: static/eager mappings with limited isolation and no mandatory demand paging.
- `PROFILE_MPU_ONLY`: region isolation semantics only; no sparse paged VM promises.

> Rule: MPU-only and MMU paths must not share fake "compatibility" semantics.

## 4) Cross-architecture acceptance criteria

Every architecture backend (`x86_64`, `arm64`, `riscv64`) must satisfy:

- create/destroy address-space root,
- map/unmap/protect/query range,
- encode permissions and cache attributes correctly,
- local + remote TLB invalidation contract,
- kernel/user split invariants,
- page-table teardown with no leaks.

### x86_64 specifics

- 4-level required, 5-level abstraction-ready.
- 4 KiB + 2 MiB mappings required, 1 GiB optional.
- NX/U/S/W/Global/PAT handling and PCID-aware switching.

### arm64 specifics

- stage-1 descriptor correctness for configured VA/granule.
- MAIR + shareability + device memory attributes.
- break-before-make remap path.
- ASID-aware context switching.

### riscv64 specifics

- Sv39 required, Sv48 abstraction-ready.
- V/R/W/X/U/G/A/D handling with hardware capability checks.
- `satp` switching + `sfence.vma` local/remote discipline.

## 5) Implementation backlog (execute in order)

## Phase 0 — Contract and layout cleanup

- [ ] Create/normalize directory boundaries: `pmm/`, `vm/objects/`, `vm/aspace/`, `vm/fault/`, `dma/`, `iommu/`, `hal/mmu/`.
- [x] Add public headers:
  - [ ] `include/mm/pmm.h`
  - [ ] `include/mm/vm_object.h`
  - [ ] `include/mm/aspace.h`
  - [ ] `include/mm/fault.h`
  - [ ] `include/mm/dma.h`
  - [ ] `include/hal/hal_pt.h`
  - [ ] `include/hal/hal_tlb.h`
  - [ ] `include/hal/hal_iommu.h`
- [ ] Define stable API contracts and ownership rules between layers.

## Phase 1 — PMM hardening

- [ ] Normalize firmware/bootloader memory map ingestion.
- [ ] Add page metadata (`page_frame_t`) with refcount, flags, owner class.
- [ ] Add DMA-capable zones and low-memory constraints.
- [ ] Add contiguous allocation + pin/unpin hooks.
- [ ] Add optional NUMA node tagging in metadata.
- [ ] Add PMM invariants tests (allocation/free/refcount/leak checks).

## Phase 2 — Hardware page-table manager completion

- [ ] Implement architecture-neutral page-table API (`hal_pt_*`).
- [ ] Complete runtime map/unmap/protect/query flows for each arch.
- [ ] Implement split/merge support for large pages.
- [ ] Implement local + remote TLB shootdown APIs (`hal_tlb_*`).
- [ ] Implement safe teardown path with accounting.
- [ ] Add arch MMU conformance tests (permissions, faults, teardown).

## Phase 3 — Address spaces and VM objects

- [ ] Introduce `address_space_t` per process/container.
- [ ] Implement region tree/interval map with overlap protection.
- [ ] Introduce `vm_object_t` kinds: anon/shared/file/device/dma.
- [ ] Add region attach/detach and inheritance semantics.
- [ ] Wire authoritative VA→region→object lookup path.
- [ ] Add clone/fork scaffolding for later COW.

## Phase 4 — Fault engine and demand paging

- [ ] Implement fault decoder: not-present vs permission vs access type.
- [ ] Implement zero-fill-on-demand for anonymous objects.
- [ ] Implement COW break path and write-fault handling.
- [ ] Implement bounded stack growth policy.
- [ ] Add pager callback contract for file/pager-backed objects.
- [ ] Define recover/kill/escalation policy per profile.

## Phase 5 — NUMA and topology policies (Tier B/C)

- [ ] Add node/domain abstraction and topology discovery hooks.
- [ ] Add allocation policies: local-preferred/interleave/bind/fallback.
- [ ] Add scheduler↔memory affinity hints.
- [ ] Add migration hooks and huge-page node policy.
- [ ] Add observability metrics for locality and remote-access cost.

## Phase 6 — DMA/IOMMU memory lifecycle

- [ ] Implement `dma_buffer_object` + pin budget accounting.
- [ ] Add coherent vs streaming DMA APIs.
- [ ] Add IOVA allocator and per-device domain model.
- [ ] Add cache maintenance/sync primitives.
- [ ] Add backend stubs/contracts for VT-d, SMMU, RISC-V platform variants.
- [ ] Add bounce-buffer fallback for low-end/non-IOMMU profiles.

## 6) Task slicing (recommended issue sequence)

Use this sequence so each item is independently reviewable and testable:

1. **MM contracts and header skeletons**
2. **PMM metadata + refcounting**
3. **PMM DMA zones + contiguous alloc**
4. **Common `hal_pt` interface + x86_64 mapping parity**
5. **arm64 descriptor/attribute parity + BBM flow**
6. **riscv64 Sv39 parity + `sfence.vma` discipline**
7. **`address_space_t` and region tree**
8. **`vm_object_t` (anon/shared/file/device/dma) base ops**
9. **Fault decode + demand-zero**
10. **COW and fork cloning**
11. **Pager/file-backed object path**
12. **TLB shootdown SMP stress tests**
13. **NUMA policy framework + metrics**
14. **DMA buffer lifecycle + IOMMU domain abstraction**

## 7) Definition of done (DoD)

A memory subsystem milestone is complete only if all are true:

- [ ] API contract documented and merged.
- [ ] Unit/integration tests added and green in CI for applicable targets.
- [ ] No mapping/refcount leaks under stress tests.
- [ ] Fault behavior is deterministic and profile-compliant.
- [ ] Architecture notes updated (`x86_64`, `arm64`, `riscv64`, `mpu-only`).
- [ ] Observability added (counters/traces for map/unmap/fault/TLB shootdown).

## 8) Immediate next 3 tasks (start now)

1. Land memory-layer public headers + ownership contracts.
2. Land PMM page metadata with refcount + DMA zone tagging.
3. Land architecture-neutral `hal_pt` interface and x86_64 conformance first.

These three unblock all later VM-object, fault, and pager work.
