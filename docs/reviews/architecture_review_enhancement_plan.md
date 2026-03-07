# Architecture Review and Enhancement Plan

## Executive Summary

This document provides a comprehensive code review and enhancement plan for Bharat-OS, focusing on Memory Management, Process and Thread Management, DMA, IPC, Async, Timers, and Hardware Accelerators/ISA Extensions. The OS currently features a solid architectural baseline adhering to a capability-based microkernel design with strong NUMA awareness and a fail-fast IPC model. However, several subsystems remain in an early scaffolding stage and require significant depth to achieve production readiness.

---

## 1. Memory Management (MM)

### Current State
- **PMM (`pmm.c`)**: Implements a buddy allocator (`MAX_ORDER` 12) with NUMA node awareness (`MAX_NUMA_NODES` 4). Parses Multiboot2 tags on x86_64 to map available memory.
- **VMM (`vmm.c`)**: Currently acts as an architecture-neutral software mapping table (`vmm_mapping_t` registry) rather than a true hardware page-table manager.
- **NUMA**: The PMM successfully attempts local node allocation before falling back to reclaiming pages or searching other nodes.

### Strengths
- **Locking & Concurrency**: Uses atomic operations and spinlocks appropriately within the PMM.
- **Buddy System**: Implemented correctly for order-based coalescing and splitting.
- **CoW Support**: Reference counting exists to support Copy-on-Write semantics.

### Issues & Gaps
- **Architecture-Specific Paging Missing**: The VMM does not program actual hardware MMUs (e.g., x86_64 4-level paging, RISC-V Sv39/Sv48, or ARM64 TTBR). The `vmm_map_page` just updates a software array and flushes the TLB.
- **Memory Reclaim Logic**: `pmm_reclaim_one_node` relies on a fixed `PMM_RECLAIM_BATCH` pool, but the logic for populating this pool (eviction/swapping) is not yet implemented.
- **Static Arrays**: `kernel_mappings` is statically sized to `VMM_MAX_MAPPINGS` (2048), severely limiting the number of virtual mappings.

### Recommendations for Next Phase
- **Implement Hardware Page Tables**: Transition VMM to build and manage architecture-specific page directory structures.
- **Dynamic VMM Allocation**: Replace the static mapping array with dynamic page-table allocations.
- **Slab/Object Allocator**: Implement a slab allocator (e.g., `kmalloc`) on top of the PMM buddy allocator for small kernel objects (threads, capabilities, IPC messages).

---

## 2. Process and Thread Management

### Current State
- **Core Scheduler (`sched_stub.c`)**: Provides a basic round-robin scheduler with EDF support. Thread and process slots are statically allocated.
- **AI Governor (`ai_sched.c`)**: A user-space AI governor bridge collects telemetry (CPI, cache misses, IPC latency) and feeds heuristics back to the scheduler.
- **SMP**: Basic multi-core structures exist (`g_cores`), but per-core runqueues are missing.

### Strengths
- **AI Integration**: The `ai_kernel_bridge` provides a clean, telemetry-driven API without polluting Ring-0 with complex ML models, adhering to the "Policy Out, Mechanism In" philosophy.
- **NUMA Thread Migration**: Threads can be assigned a preferred NUMA node.

### Issues & Gaps
- **Global Lock/State**: The scheduler relies heavily on global state (`g_threads`, `g_processes`, `g_current`) without fine-grained locking, making SMP scaling impossible.
- **Context Switching**: `fv_secure_context_switch` is declared `weak` and not implemented in assembly for the target architectures. The CPU state is not actually saved/restored correctly during `sched_yield`.
- **Static Limitations**: Hard limits on threads (64) and processes (32) are suitable for RTOS/Edge but not for Desktop/Server profiles.

### Recommendations for Next Phase
- **Per-Core Runqueues**: Refactor the scheduler to use per-CPU runqueues with lockless or fine-grained spinlock synchronization.
- **Implement Context Switching**: Write the architecture-specific assembly routines (x86_64, RISC-V, ARM64) to save and restore registers.
- **Dynamic Thread Allocation**: Move thread/process objects to the new slab allocator (see MM recommendations).

---

## 3. IPC (Inter-Process Communication) and Async

### Current State
- **Endpoints (`endpoint_ipc.c`)**: Implements synchronous, capability-guarded endpoints (`ipc_endpoint_t`) using a static array.
- **Multikernel/URPC**: Shared memory ring buffers (Lockless URPC) exist for high-performance, cross-core, and AI Governor messaging.

### Strengths
- **Capability Security**: IPC operations explicitly require `CAP_PERM_SEND` or `CAP_PERM_RECEIVE`.
- **Bounded Queues**: Prevents unbounded memory growth in the kernel by keeping IPC message sizes fixed or utilizing ring buffers.

### Issues & Gaps
- **Synchronous Blocking**: Endpoint IPC blocks the current thread (`THREAD_STATE_BLOCKED`), but the wakeup mechanism in `ipc_endpoint_receive` is brittle (assumes the blocked sender is `g_current`, which is impossible if the sender is actually blocked and not running).
- **Async Events/Signals**: No formal asynchronous notification or signal mechanism exists for user-space to handle hardware interrupts or exceptions.

### Recommendations for Next Phase
- **Fix IPC Blocking Logic**: Implement proper wait queues for endpoints so threads can be suspended and precisely awoken when a message is sent or received.
- **Async Event Capabilities**: Introduce an "Event" or "Notification" capability object to allow asynchronous signaling without full message payload transfer.

---

## 4. Timers and HAL (Hardware Abstraction Layer)

### Current State
- **Common Timer (`timer_common.c`)**: Tracks monotonic ticks driven by `hal_timer_tick`.
- **HAL APIs**: Defined in `hal.h`, with stubs for `x86_64`, `riscv`, and `arm64`.

### Strengths
- **Clean Abstraction**: The boundary between kernel logic and architecture-specific HAL is well-defined.

### Issues & Gaps
- **Unimplemented Hardware Initialization**: `hal_cpu.c` across all architectures is full of `TODO`s. For example, x86_64 does not configure the APIC/IOAPIC; RISC-V does not configure PLIC/CLINT.
- **Timer Driven Scheduling**: `sched_on_timer_tick` is called, but the hardware timer interrupts are not actually wired up to trigger it dynamically.

### Recommendations for Next Phase
- **Wire up Interrupt Controllers**: Implement IOAPIC/Local APIC (x86_64), PLIC/CLINT (RISC-V), and GIC (ARM64).
- **Configure Hardware Timers**: Implement periodic timer initialization (e.g., APIC timer, RISC-V `timecmp`) to drive the preemption tick.

---

## 5. DMA, Accelerators, and ISA Extensions

### Current State
- **Device Framework (`device_manager.c`, `builtin_drivers.c`)**: Registers basic MMIO windows for devices.
- **VMM Device Mapping**: `vmm_map_device_mmio` explicitly checks for `CAP_RIGHT_DEVICE_NPU` or `CAP_RIGHT_DEVICE_GPU` capabilities before mapping memory.

### Strengths
- **Capability-Driven MMIO**: Access to NPU/GPU memory is strictly mediated by the capability system, ensuring strong isolation for accelerators.

### Issues & Gaps
- **No DMA Allocator**: There is no API to allocate contiguous physical memory with specific caching attributes (e.g., Uncacheable, Write-Combining) required for DMA operations.
- **ISA Extension Contexts**: The scheduler context (`cpu_context_t`) does not currently account for extended processor states like x86_64 AVX/SSE, RISC-V Vector (V-extension), or ARM64 NEON. This will corrupt state if floating-point/vector ops are used in user-space.

### Recommendations for Next Phase
- **Implement DMA Memory Allocation**: Extend the MM subsystem with `mm_alloc_dma_pages()` that configures proper memory types via page table attributes (PAT/MTRR on x86, PMA/PMP on RISC-V).
- **Extended Context Save/Restore**: Update thread context structures and assembly routines to save/restore FPU and Vector registers (using `XSAVE`/`XRSTOR` on x86_64). Optimize this by using lazy FPU context switching.