# Bharat-OS Memory Architecture

## 1. Executive Summary

Bharat-OS implements a capability-gated multikernel memory architecture that enforces strict separation between mechanisms (physical memory allocation, hardware translation) and policies (virtual memory semantics, demand paging). The memory subsystem has matured beyond the bootstrap phase and provides a robust foundation encompassing physical memory management (PMM), virtual memory objects, address spaces, and hardware translation through a capability-aware HAL.

The primary design principle is **strict ownership**: no layer may reach around its adjacent layer. This prevents architecture leakage, ensures profile truthfulness (e.g., distinguishing between MMU and MPU contexts), and guarantees determinism.

---

## 2. Layered Architecture and Ownership Boundaries

The memory stack is divided into explicit layers, each with strict responsibilities:

| Layer | Responsibility | Must Not Know About |
|---|---|---|
| **PMM** (`kernel/src/mm/pmm/`) | Physical frames, zones, buddy allocation, contiguous alloc, refcounts | Virtual memory policies, address spaces, demand faults |
| **VM Object** (`kernel/src/mm/vm/objects/`) | Backing semantics (anon, file, shared, device), lifecycle, page fault resolution | Hardware translation formats, page tables |
| **Address Space** (`kernel/src/mm/vm/aspace/`) | Virtual memory region reservations, overlap rules, object attachment | Physical allocation policies |
| **HAL PT** (`kernel/include/hal/hal_pt.h`) | Hardware page table programming, architecture attributes | VM object semantics |
| **HAL TLB** (`kernel/src/mm/tlb/`) | Invalidation coordination (local/remote SMP shootdown) | PMM frame ownership, aspace lookup |
| **Fault Engine** (`kernel/src/mm/vm/fault/`) | Decoding traps, orchestrating lookup, invoking object faults, repairing backend | PMM internal structures |
| **DMA/IOMMU** (`kernel/src/mm/dma/`) | Device-visible mappings, IOVA domains, cache maintenance | General user-space memory semantics |

### Layer Ownership Map

```mermaid
flowchart TD
    A[User / Kernel Requests] --> B[Address Space Manager]
    B --> C[VM Region Metadata]
    C --> D[VM Object Layer]
    D --> E[Fault Engine / Pager Hooks]
    D --> F[PMM]
    B --> G[HAL Page Table Wrapper]
    G --> H[Arch PT Backend]
    H --> I[HAL TLB Layer]
    I --> J[Local Flush / Remote Shootdown]
    D --> K[DMA / IOMMU Layer]
    K --> L[Device-visible mappings / IOVA]
    F --> M[Zones / Refcount / Pin / Contig]
```

---

## 3. Fault Resolution Sequence

The unified fault engine provides a deterministic state machine for handling memory faults across all architectures.

```mermaid
sequenceDiagram
    participant CPU
    participant Trap as Trap Handler
    participant Fault as Fault Engine
    participant A as Address Space
    participant O as VM Object
    participant P as PMM
    participant PT as HAL PT
    participant T as HAL TLB

    CPU->>Trap: page/access fault
    Trap->>Fault: vm_handle_fault(event)
    Fault->>A: lookup region/object
    A-->>Fault: region + object
    Fault->>O: resolve fault (obj->ops->fault)
    O->>P: allocate/pin backing if needed
    P-->>O: frame
    O-->>Fault: resolved page + flags
    Fault->>PT: map/protect entry
    PT->>T: invalidate if needed
    Fault-->>Trap: VM_FAULT_RESOLVED / VM_FAULT_KILL
```

---

## 4. Profile Behavior Model

Bharat-OS operates across different device classes and enforces memory behavior guarantees natively without emulating unsupported capabilities.

| Capability | Profile: MMU-Full (e.g. Datacenter, Desktop) | Profile: MMU-Lite (e.g. Edge, Drone) | Profile: MPU-Only (e.g. Embedded RTOS) |
|---|---|---|---|
| **Page Mapping** | Full capability | Backend-dependent (fallback allowed) | Unsupported (sparse-page operation) |
| **Range Mapping** | Full capability | Wrapper fallback allowed | Region-programming only |
| **Protection** | Full granularity | Partial/Eager | Explicitly unsupported for page semantics |
| **Demand Faults**| Fully supported | Limited / Eagerly resolved | Not supported |
| **COW** | Software-managed | Limited | Not supported |
| **Huge Pages** | Capability-driven | Often disabled | N/A |
| **Device Maps** | Yes | Attribute-dependent | Region attribute only |
| **Recovery** | Fine-grained | Degraded path | Region violation (KILL) |

---

## 5. TLB SMP Shootdown Coordination

The multikernel architecture utilizes a synchronous/asynchronous invalidation loop depending on target requirements, leveraging uRPC for inter-core communication to maintain TLB coherency without introducing global locking bottlenecks.

```mermaid
flowchart LR
    A[Core A Unmap/Protect] --> B[Increment ASpace Generation]
    B --> C[Collect Active Cores]
    C --> D[Send Targeted uRPC Invalidation]
    D --> E[Remote Core Flushes Local TLB]
    E --> F[Remote Core Sends ACK]
    F --> G[Sender Marks Request Complete]
```

---

## 6. Implementation Checklist & Conformance

Every architecture backend (`x86_64`, `arm64`, `riscv64`, `arm32`, `riscv32`) must satisfy the following memory guarantees:

| Check | x86_64 | arm64 | riscv64 | arm32 / riscv32 MPU |
|---|---|---|---|---|
| create/destroy aspace root | ☐ | ☐ | ☐ | ☐ |
| map/unmap page | ☐ | ☐ | ☐ | n/a |
| map/unmap range | ☐ | ☐ | ☐ | region-only |
| protect/query | ☐ | ☐ | ☐ | explicitly unsupported |
| user/kernel split invariants | ☐ | ☐ | ☐ | ☐ |
| local TLB invalidate | ☐ | ☐ | ☐ | n/a |
| remote TLB invalidate | ☐ | ☐ | ☐ | n/a |
| teardown leak-free | ☐ | ☐ | ☐ | ☐ |
| fault policy profile-correct | ☐ | ☐ | ☐ | ☐ |

---

## 7. Current State and Open Backlog

- **PMM**: Core zoned allocator, early allocator, and contiguous allocators exist. Needs deeper NUMA domain hardening.
- **VM Objects**: Anonymous, Shared, File, Device, and DMA kinds are structurally implemented with rigorous lifecycle validation.
- **ASpace**: Interval trees and base APIs are mature.
- **Fault Contract**: Unified `vm_handle_fault` contract implemented and host-tested (including OOM, segmentation, permissions, and backing resolution stages).
- **HAL Translation**: Interfaces (`hal_pt`) exist but need full translation across all architectures (specifically 32-bit backends).
- **DMA/IOMMU**: Scaffolding is complete but real SMMU/VT-d integration and lifecycle caching remain open.