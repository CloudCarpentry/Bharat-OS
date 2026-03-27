# Bharat-OS Comprehensive Memory Architecture

## 1. Executive Summary

Bharat-OS implements a capability-gated multikernel memory architecture that enforces strict separation between mechanisms (physical memory allocation, hardware translation) and policies (virtual memory semantics, demand paging). The memory subsystem provides a robust foundation encompassing physical memory management (PMM), virtual memory objects, address spaces, and hardware translation through a capability-aware HAL.

The primary design principle is **strict ownership**: no layer may reach around its adjacent layer. This prevents architecture leakage, ensures profile truthfulness (e.g., distinguishing between MMU and MPU contexts), and guarantees determinism.

## 2. Core Principles & Memory Model

The monolithic memory stack is split into a **Minimal Memory Core** (always included: PMM, MPU-lite, early alloc) and an **Advanced VM** tier (conditionally included: address spaces, demand paging, COW, NUMA). This ensures embedded builds do not link or initialize complex VMM structures.

```mermaid
flowchart TD
    A[Build Configuration / Presets] --> B{Capabilities}
    B -->|BHARAT_ENABLE_ADVANCED_VM=ON| C[Advanced VM Tier]
    B -->|BHARAT_ENABLE_ADVANCED_VM=OFF| D[Minimal Core Only]

    C --> E(Address Spaces, COW, Demand Paging, NUMA)
    D --> F(PMM, Frame Allocator, MPU-Lite, Fixed Mappings)

    E --> F

    G[Memory Allocations] --> H{alloc_class_t}
    H -->|MEM_NORMAL, MEM_RT, MEM_PACKET| D
    H -->|MEM_DMA, MEM_SECURE| I[Specialized Pools / CMA]
```

### 2.1 PMM Invariants

The Physical Memory Manager (PMM) operates under strict state invariants.

```mermaid
stateDiagram-v2
    [*] --> PMM_PAGE_STATE_BOOT: Early Boot
    PMM_PAGE_STATE_BOOT --> PMM_PAGE_STATE_FREE: mm_init_zones()

    PMM_PAGE_STATE_FREE --> PMM_PAGE_STATE_ALLOCATED: mm_alloc_page()
    PMM_PAGE_STATE_ALLOCATED --> PMM_PAGE_STATE_PINNED: mm_pin_page()
    PMM_PAGE_STATE_PINNED --> PMM_PAGE_STATE_ALLOCATED: mm_unpin_page()
    PMM_PAGE_STATE_ALLOCATED --> PMM_PAGE_STATE_FREE: mm_free_page()
```

- **Boot State**: Pages are tracked but not available.
- **Free State**: Pages are in buddy lists.
- **Allocated State**: Active use. Reference counts handle multiple references.
- **Pinned State**: Locked physically (e.g., for DMA), eviction is prevented.

## 3. Layered Architecture and Ownership Boundaries

The memory stack is divided into explicit layers, each with strict responsibilities:

| Layer | Responsibility | Must Not Know About |
|---|---|---|
| **PMM** (`kernel/src/mm/pmm/`) | Physical frames, buddy allocation, contiguous alloc | Virtual memory policies, demand faults |
| **VM Object** (`kernel/src/mm/vm/objects/`) | Backing semantics, lifecycle, page fault resolution | Hardware translation formats |
| **Address Space** (`kernel/src/mm/vm/aspace/`) | Region reservations, overlap rules | Physical allocation policies |
| **HAL PT** (`kernel/include/hal/hal_pt.h`) | Hardware page table programming | VM object semantics |
| **HAL TLB** (`kernel/src/mm/tlb/`) | Invalidation coordination | PMM frame ownership |
| **Fault Engine** (`kernel/src/mm/vm/fault/`) | Decoding traps, orchestrating lookup | PMM internal structures |
| **DMA/IOMMU** (`kernel/src/mm/dma/`) | Device-visible mappings, IOVA domains | General user-space semantics |

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

## 4. Fault Resolution Sequence

The unified fault engine provides a deterministic state machine for handling memory faults.

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

## 5. Profile Behavior Model

Bharat-OS operates across different device classes:

| Capability | MMU-Full (Datacenter) | MMU-Lite (Edge) | MPU-Only (RTOS) |
|---|---|---|---|
| **Page Mapping** | Full capability | Backend-dependent | Unsupported |
| **Protection** | Full granularity | Partial/Eager | Explicitly unsupported |
| **Demand Faults**| Fully supported | Limited / Eagerly resolved | Not supported |
| **COW** | Software-managed | Limited | Not supported |

```mermaid
flowchart LR
    A[Build Configuration] -->|BHARAT_KERNEL_PROFILE| B{Kernel Profile}
    B -->|PROFILE_MMU_FULL| C[Full VM, Paging, COW, Shared Mem]
    B -->|PROFILE_MMU_LITE| D[Static Mappings, No Demand Paging]
    B -->|PROFILE_MPU_ONLY| E[Region Isolation, No Paging]

    C --> C1[x86_64, arm64, riscv64]
    D --> D1[arm32, riscv32]
    E --> E1[Cortex-M, RV32IMAC]

    style C fill:#d5f5e3,stroke:#333
    style D fill:#fdebd0,stroke:#333
    style E fill:#f5b7b1,stroke:#333
```

## 6. Hardware Abstraction and Interfaces

The OS communicates with the physical architecture using a capability-aware HAL. It handles page tables (`hal_pt_map`, `hal_pt_unmap`), TLB invalidations, IOMMU translations, and MMIO mappings.

### TLB SMP Shootdown Coordination

```mermaid
flowchart LR
    A[Core A Unmap/Protect] --> B[Increment ASpace Generation]
    B --> C[Collect Active Cores]
    C --> D[Send Targeted uRPC Invalidation]
    D --> E[Remote Core Flushes Local TLB]
    E --> F[Remote Core Sends ACK]
    F --> G[Sender Marks Request Complete]
```

### 6.1 Monitor VM Channel Coordination

The distributed VM implementation relies on the Monitor VM Channel for inter-core memory coordination. The monitor (`kernel/src/monitor/mon_vm_service.c` and `mon_vm_dispatch.c`) is strictly a mechanism layer, providing bounded-completion URPC/IPC capabilities to the advanced VM and TLB controllers.

**Design Guarantees:**
1. **Bounded Wait:** Operations are dispatched with strict limits on wait times (e.g., `mon_vm_wait_for_acks` terminates on timeout or terminal error rather than indefinite blocking).
2. **Contract Discipline:** Dispatching map/unmap/protect requests validates message envelopes, alignments, and flags before taking action. Malformed requests are explicitly nacked (`MON_VM_STATUS_MALFORMED`).
3. **Observability:** Telemetry metrics (`stat_requests_sent`, `stat_acks_received`, `stat_timeouts`, etc.) track cross-core coordination health, surfacing channel issues quickly.

## 7. Memory Operations Layering

Memory copying, zeroing, and moving follows strict layering:

```mermaid
flowchart TD
    A[High-level `memcpy`/`memset`] --> B{Address Space Check}
    B -->|User| C[User-space copy logic (`copy_to_user`)]
    B -->|Kernel| D[Kernel memory routines]

    C --> E[Fault-safe memory operation]
    D --> F[Generic memory operation]

    E --> G[Hardware specific intrinsics (e.g. rep movsb)]
    F --> G
```

### 7.1 Syscall Map Region Example

For contrast, mapping a region follows a different path involving the HAL PT and TLB layers:

```mermaid
sequenceDiagram
    participant UserApp as User Space App
    participant VMM as VMM/ASpace
    participant HAL_PT as HAL Page Table Layer
    participant TLB_Coordinator as TLB Coordinator
    participant Hardware as MMU/MPU

    UserApp->>VMM: Syscall Map Region
    VMM->>HAL_PT: hal_pt_map(phys, virt, flags)
    HAL_PT->>Hardware: Write PTE / Region
    Hardware-->>HAL_PT: Ack
    HAL_PT-->>VMM: Success
    VMM->>TLB_Coordinator: hal_tlb_flush(virt)
    TLB_Coordinator->>Hardware: Local Flush
    TLB_Coordinator->>Hardware: Remote Shootdown (IPI)
    Hardware-->>TLB_Coordinator: Ack
    TLB_Coordinator-->>VMM: Ack
    VMM-->>UserApp: Complete Map
```

## 8. Debugging & Benchmarking Strategy

Memory correctness is verified using:
1. **Valgrind/Memcheck** on host tests.
2. **KASAN / UBSAN** inside the kernel.
3. **Lockdep** for multithreaded debugging.

Benchmark metrics cover VM allocations, DMA throughput, and HAL latencies across architectures.
