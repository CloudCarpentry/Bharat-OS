# ADR-021: Hardware Capability Discovery, Aggregation and Freeze Authority

- **Status:** Accepted
- **Date:** 2026-08-09

## Context

Architecture backend declarations, per-CPU probes, and the HAL hardware snapshot previously acted as overlapping authorities.  In particular, an unavailable system CPU record appeared to HAL consumers as a successfully initialized empty set, and the primitive registry ran before CPU aggregation.  Several architecture defaults also claimed platform facts such as IOMMU presence or DMA coherency without ACPI, FDT, or board evidence.

## Decision

Capability meanings are separated as follows:

1. `arch_get_caps()` describes the selected architecture/profile mechanism contract.  It is not platform discovery.
2. `arch_cpu_caps_*` records CPU-register or ISA-discovery facts.  `LOCAL` is the current CPU, `SYSTEM_ALL` is the intersection across participating CPUs, and `SYSTEM_ANY` is their union.
3. HAL discovery owns platform facts such as topology and IOMMU presence.  Unknown platform facts are false.  The architecture hardware publishers provide conservative raw input and do not infer IOMMU or DMA coherency from ISA.
4. `hal_hw_caps_t` is the effective snapshot consumed by mechanisms.  It moves monotonically through `UNINITIALIZED`, `RAW_DISCOVERED`, `CPU_FINALIZED`, and `FROZEN`.  The BSP boot path is its sole writer; after `FROZEN`, callers receive only a const pointer and every mutation/finalization attempt fails.
5. Memory backend support remains distinct from the effective memory profile.  The snapshot reports the selected effective mechanism; supporting an MMU backend does not itself select `MMU_FULL`.
6. The primitive registry accepts only the exact frozen HAL snapshot, normalizes it once, and rejects early or repeated initialization.  Generic migratable paths consume `SYSTEM_ALL`; specialized placement may inspect `SYSTEM_ANY`; CPU-local dispatch rechecks `LOCAL`.

The implemented boot sequence is:

```text
hal_init / architecture raw publication
        |
HAL platform and topology discovery
        |
per-CPU architecture discovery
        |
SYSTEM_ALL intersection + SYSTEM_ANY union
        |
publish CPU facts into effective HAL snapshot
        |
publish platform facts and FREEZE
        |
primitive registry normalization (exactly once)
        |
algorithm backends, scheduler, and services
```

Profile validation remains the existing boot responsibility; this decision does not redesign memory policy.  The freeze boundary ensures later primitive and backend binding cannot observe partially aggregated hardware truth.

## Failure behavior and synchronization

Lifecycle transitions are fail closed: a missing CPU aggregate returns `K_ERR_IN_PROGRESS`; an out-of-order, backward, repeated, or post-freeze transition returns `K_ERR_BAD_STATE`.  No partial transition is published.  Mutation is BSP-owned during serial boot, then the object is immutable shared state, so no runtime lock is required.

The generic discovered CPU count is bounded by the canonical `BHARAT_MAX_CPUS` rather than 32.  Existing 32-bit service-placement masks remain explicitly limited to CPUs 0 through 31; they no longer truncate the authoritative topology count.  Expanding that compatibility interface is a separate UAPI/service migration.

## Consequences

- An initialized empty CPU feature set is distinguishable from an unavailable aggregate.
- Heterogeneous systems cannot accidentally enable an `ANY` instruction in migratable generic code.
- IOMMU and DMA-coherency claims require platform evidence; absent evidence disables them.
- Existing consumers that attempted early hardware dispatch now fail and must move after freeze.
- Trap entry, syscalls, capabilities, scheduling policy, uRPC, TLB protocols, and memops are unchanged.
