---
title: Hybrid Shared-Memory and Multikernel Concurrency Model with Profile-Aware Boot Handoff
status: Accepted
owner: Kernel Architecture and System Services Working Group
last_updated: 2026-10-15
---

# ADR-039: Hybrid Shared-Memory and Multikernel Concurrency Model with Profile-Aware Boot Handoff

## 1. Context and Problem Statement

Bharat-OS employs a capability-based per-core multikernel architecture to maintain strict deterministic ownership, eliminate global lock contention, and enable formal verification. However, a naive multikernel implementation where every cross-core thread interaction requires kernel-level inter-core messaging introduces severe latency and overhead for multithreaded applications and low-latency synchronization (e.g., futexes, shared memory IPC, work-stealing thread pools).

Furthermore, the boot handoff mechanism from root `init` through `process_manager` must support diverse hardware and memory protection tiers:
- **`MMU_FULL`**: 64-bit architectures with multi-level page tables, demand paging, and full userland virtual memory management (`vm_manager`).
- **`MMU_LITE`**: 32-bit systems (arm32, riscv32) with segmented/windowed protection without page tables.
- **`MPU/PMP`**: Constrained microcontrollers and deterministic RTOS profiles using hardware region registers.
- **Single-Application / Tiny Profiles**: Constrained firmware deployments where full service hierarchies are bypassed in favor of direct application execution.

## 2. Decision and Architecture

### 2.1 Three-Tier Concurrency Architecture

We adopt a three-tier concurrency model preserving kernel per-core ownership while providing zero-overhead shared-memory multicore execution for user applications:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        User Space Applications                         │
│                                                                        │
│   Thread A (Core 0)                      Thread B (Core 1)             │
│          │                                      │                      │
│          └─────────── Coherent Shared Memory ───┴──────────┐           │
│                       (Atomics, Futexes, Lock-Free Ring)   │           │
│                                                            ▼           │
│                                                   Tier 2: Fast Path    │
├────────────────────────────────────────────────────────────┼───────────┤
│                     Bharat-OS Kernel Layers                │           │
│                                                            │           │
│   Core 0 Kernel                          Core 1 Kernel     │           │
│   ┌─────────────────────┐                ┌─────────────────────┐       │
│   │ Local Scheduler     │                │ Local Scheduler     │       │
│   │ pmm_pcache Magazine │                │ pmm_pcache Magazine │       │
│   └─────────────────────┘                └─────────────────────┘       │
│     Tier 1: Core-Local                     Tier 1: Core-Local          │
│     Fast Path                              Fast Path                   │
│               ▲                                      ▲                 │
│               └──────── Tier 3: Control Path ────────┘                 │
│                         (uRPC, Capability Grants,                      │
│                          Monotonic Deadlines)                          │
└────────────────────────────────────────────────────────────────────────┘
```

1. **Tier 1: Local Fast Path (Kernel Core-Local)**:
   - Scheduling, runqueues, local memory caches (`pmm_pcache` magazines), and handle tables are strictly core-local.
   - Operations on core-local state execute with zero cross-core locking or bus locks.
2. **Tier 2: Shared-Memory Concurrency Fast Path (Userland)**:
   - Threads belonging to the same address space or sharing memory objects access coherent memory directly via hardware-supported cache coherence.
   - Userland synchronization primitives (mutexes, semaphores, condition variables) execute entirely in userspace using atomic instructions (`atomic_compare_exchange`).
   - Contention fallbacks invoke the kernel personality futex mechanism (`linux_futex_dispatch`), protected by per-bucket spinlocks, atomic verify-under-lock checks (eliminating lost wakeups), and monotonic deadlines (`sched_sleep` / `sched_block`).
3. **Tier 3: Cross-Core Control Path (Bounded Message Protocols)**:
   - Structural mutations (thread migration, address space creation, capability delegation, TLB invalidation) use bounded, non-blocking message-passing protocols with monotonic deadlines (`bh_ktime_now()`).
   - No cross-core pointer crossing or shared mutable kernel state.

### 2.2 Profile-Aware Boot Executable Authority Handoff

The boot handoff pipeline progresses through:
`INIT -> NAMESVC (Root) -> PROCESS_MANAGER -> SERVICES / APPS`

1. **Tri-Model Memory Protection Gating**:
   - **`MMU_FULL` (x86_64, ARM64, RISC-V 64)**:
     - Full virtual memory abstractions (`aspace -> region -> vm_object`) with multi-level hierarchical paging (PML4, 4-level translation).
     - Full capability set enabled (`BHARAT_INIT_CAP_MMU`). Launches full dynamic service graph including `vm_manager`.
     - User applications own isolated lower-half address spaces, mapped dynamically using capability-mediated page-table construction.
   - **`MMU_LITE` (ARM32 Cortex-A/R, RISC-V 32)**:
     - Coarse-grained page or section-based translation with fixed/flat mappings and no demand-paging.
     - `BHARAT_INIT_CAP_MMU` is explicitly masked in `init_profile.c`. `filter_service()` automatically excludes `vm_manager`, routing memory management directly through kernel base/limit and region mechanisms.
     - Services and processes run within deterministic, non-demand-paged memory blocks allocated during boot reservation.
   - **`MPU_ONLY` / PMP (ARM Cortex-M/R MPU, RISC-V PMP)**:
     - Hardware protection relies on a bounded set of physical memory regions (8, 16, or 32 regions).
     - No virtual address translation exists (`VA == PA`). Region base and limit registers enforce execution and access permissions (`RO/X` for `.text`, `RO` for `.rodata`, `RW/NX` for `.data/.bss/stack`).
     - Memory allocation uses contiguous physical blocks (`bh_alloc_region`). Dynamic virtual address mapping and page-table traversal APIs fail closed (`BH_STATUS_ERR_UNSUPPORTED`).
     - `init` identifies MPU hardware via `hal_mem_caps()` and masks all paging capabilities, ensuring memory management executes statically without `vm_manager`.
2. **Constrained & Single-Application Profiles**:
   - In constrained profiles (`INIT_PROFILE_TINY`, `INIT_PROFILE_SMALL`, `INIT_PROFILE_RT`, or single-application mode):
     - Multi-tier service orchestration (`servicemgr`, `vm_manager`, `devmgr`) is stripped from the boot graph.
     - Boot context initializes minimal capability masks (`BHARAT_INIT_CAP_NONE` or device-specific peripheral caps).
     - The single target native application binary is packaged as the primary user boot module.
     - `init` hands off hardware capabilities (console, timer, interrupts, designated MPU memory regions) directly to the single application container via its startup bootstrap structure (`startup->bootstrap.system_control_endpoint`).
     - This eliminates IPC context switches and intermediary daemon overhead, achieving microsecond boot-to-application deterministic readiness.
3. **Readiness Event Transport Delegation**:
   - `init` allocates an IPC endpoint pair (`event_send`, `event_recv`) for each service candidate and transfers `event_send` to `process_manager` via `BH_PM_OP_SPAWN_V1`.
   - During `BH_PROCESS_OP_START`, the kernel delegates `event_send` to the child's CSpace and maps it to `startup->bootstrap.system_control_endpoint`.
   - The child communicates lifecycle state transitions (`BH_BOOTSTRAP_EVENT_BOUND`, `BH_BOOTSTRAP_EVENT_READY`, `BH_BOOTSTRAP_EVENT_FAILED`) directly to `init`'s receive endpoint without intermediate service polling.
4. **Graceful Optional Service Convergence**:
   - Services declared with `.policy = INIT_SERVICE_OPTIONAL` are marked `required_for_boot = false`.
   - If an optional service fails (e.g. reporting `BH_BOOTSTRAP_EVENT_FAILED` due to unsupported hardware features or uninstalled authority adapters), `init` records degraded status without halting boot convergence or triggering safe-mode rollback.


## 3. Invariants Preserved

1. **Deny by Default & Capability Mediation**: Capabilities are granted exclusively via kernel capability tables (`grant_endpoint`, `cap_table_delegate`). Raw integer handles across boundaries remain prohibited.
2. **Layer Boundary**: Kernel (`core/kernel/`) implements deterministic mechanisms; `init` and `process_manager` (`core/services/`) own policy and lifecycle orchestration.
3. **Timing Invariants**: Zero spin-count loops. All waits and timeouts use monotonic HAL nanosecond time (`bh_ktime_now()`).
4. **Wire Boundaries**: Fixed-width, compile-time asserted ABI structs (`_Static_assert`) with no pointers crossing IPC or message boundaries.
