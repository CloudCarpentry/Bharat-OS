# ADR-019: Fail-closed boot profile publication

## Status

Accepted

## Context

Boot previously treated an omitted memory-model selection as `MMU_FULL`,
advertised DMA/IOMMU capabilities as properties of that selection, allowed a
missing CPU-partition descriptor to admit any scheduling class, and published
the scheduler before resolving execution-mode partitions.  The 32-bit common
boot path also stopped on an ISA conditional rather than allowing the selected
memory backend and user-entry backend to report their actual support status.

The MPU bootstrap additionally fabricated a protection-domain pointer on
backend failure and stored it in the process address-space field.  That bypassed
the canonical process memory authority and could publish an invalid object.

## Decision

1. An omitted memory model resolves to `MEM_MODEL_NONE` and validation rejects
   it.  Model capabilities describe only guarantees intrinsic to the model;
   optional DMA/IOMMU capabilities remain HAL-discovered runtime truth.
2. The BSP resolves and validates execution mode and CPU partitions before
   scheduler global publication.  Partition lookup failure, a zero class mask,
   and an empty admission request deny admission.
3. Common boot contains no ARM32/RISC-V32 early panic.  Unsupported memory or
   user-entry behavior must be returned by the responsible backend.
4. `bh_process_t::addr_space` remains the sole process memory authority for
   MMU, MMU-Lite, and MPU.  The RT bootstrap consumes the protection domain
   owned by that address space and fails closed if it is unavailable.
5. Kernel readiness and userspace launch are diagnostic milestones, not boot
   completion.  `BOOT_COMPLETE` remains reserved for userspace-originated
   stable-runtime evidence under the boot evidence contract.

## Ownership and synchronization

The BSP is the single writer of execution configuration during boot.  After
successful initialization, readers receive a const descriptor and scheduler
publication consumes that immutable topology.  Each process owns exactly one
address space, whose backend protection domain is created and destroyed by the
address-space lifecycle.  This decision adds no remote mutation or wire
protocol.

## Failure behavior

Missing configuration, missing protection backends, invalid partitions, and
zero timer frequency are rejected before scheduler or RT-userspace
publication.  No fallback object is allocated and no capability is inferred.

## Consequences

Targets must explicitly select a memory model and provide truthful HAL
capabilities.  Bring-up configurations that previously relied on optimistic
defaults now stop at the responsible validation boundary.  Full five-target
boot claims still require executable matrix evidence; removal of an ISA panic
alone is not evidence that a target reaches userspace.
