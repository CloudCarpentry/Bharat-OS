---
title: 'BHCF-002: HMEM Architecture'
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# BHCF-002: HMEM Architecture

## Purpose
This document details the HMEM architectural component of the Bharat Heterogeneous Compute Fabric (BHCF).

## What is HMEM?

HMEM is:
> A capability-protected memory object whose contents can potentially be accessed by multiple heterogeneous execution engines without changing its application-level identity.

## Object Model
HMEM represents a conceptual object:

```text
HMEM #42
│
├── size
├── alignment
├── usage
├── properties
├── memory domain
├── capability rights
├── content generation
│
├── CPU mapping
│
├── GPU mapping
│
├── NPU mapping
│
└── DPU mapping
```
**Note:** Not all mappings need to exist simultaneously.

## Lifecycle

```mermaid
stateDiagram-v2

    [*] --> BH_HMEM_LIVE

    BH_HMEM_LIVE --> CPU_Mapped: map CPU
    CPU_Mapped --> BH_HMEM_LIVE: unmap CPU

    BH_HMEM_LIVE --> Device_Mapped: map device
    Device_Mapped --> BH_HMEM_LIVE: unmap device

    CPU_Mapped --> Synchronizing: sync for device
    Device_Mapped --> Synchronizing: sync for CPU

    Synchronizing --> BH_HMEM_LIVE

    BH_HMEM_LIVE --> BH_HMEM_DYING: final reference
    BH_HMEM_DYING --> BH_HMEM_DEAD

    BH_HMEM_DEAD --> [*]
```
(Using implementation state names `BH_HMEM_LIVE`, `BH_HMEM_DYING`, `BH_HMEM_DEAD` mapped conceptually to typical usage loops).

## Sequences

### HMEM Creation Sequence

```mermaid
sequenceDiagram

    participant A as Application
    participant S as Bharat SDK
    participant U as Native UAPI
    participant K as Kernel HMEM
    participant C as Capability Manager
    participant M as Memory Manager

    A->>S: bh_hmem_create(desc)
    S->>U: native HMEM create
    U->>K: validate request

    K->>K: validate size/alignment/flags
    K->>M: allocate backing memory
    M-->>K: backing memory

    K->>C: create capability
    C-->>K: opaque handle

    K-->>U: HMEM handle
    U-->>S: handle
    S-->>A: HMEM
```

### CPU Mapping Sequence

```mermaid
sequenceDiagram

    participant A as Application
    participant SDK as SDK
    participant K as Kernel HMEM
    participant VM as VM
    participant HAL as HAL

    A->>SDK: bh_hmem_map_cpu()
    SDK->>K: map request

    K->>K: capability check
    K->>K: access-right check

    K->>VM: establish mapping

    VM-->>K: user VA
    K-->>SDK: mapped VA

    SDK-->>A: pointer
```

**Why userspace never receives raw addresses:**
Userspace never receives:
```text
physical address
kernel virtual address
page pointer
device MMIO address
```
This is a critical security property to prevent arbitrary DMA or direct physical memory tampering. All access happens through capability-checked opaque handles and virtual addresses mapped specifically to the requesting process.

### CPU → Accelerator Synchronization Sequence

This demonstrates one of HMEM's major performance advantages.

```mermaid
sequenceDiagram

    participant APP as Application
    participant SDK as SDK
    participant K as HMEM
    participant HAL as HAL
    participant ARCH as ARCH
    participant DEV as Accelerator

    APP->>SDK: bh_hmem_sync_for_device(HMEM)
    SDK->>K: HMEM sync

    K->>K: validate range

    K->>HAL: sync range for device

    alt coherent memory
        HAL-->>K: no cache operation required
    else non-coherent memory
        HAL->>ARCH: clean cache range
        ARCH-->>HAL: complete
    end

    HAL-->>K: synchronized
    K-->>SDK: OK

    DEV->>DEV: consume buffer
```

## Layer Ownership and Capability Checks

Why do capability checks belong above HAL?

```text
Kernel:
    Is this process allowed to map HMEM to device X?

HAL:
    How do I perform this mapping?
```

Not:

```text
HAL:
    Who owns this capability?
```
Some existing accelerator HAL code currently mixes capability authorization into HAL. HMEM establishes a cleaner model where capability authorization logic remains purely in the Kernel, and HAL strictly concerns itself with portable hardware semantics.