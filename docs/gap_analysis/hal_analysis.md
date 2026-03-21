# Hardware Abstraction and Interface Layer: Gap Analysis

**Date:** 2024-05
**Scope:** Kernel, Subsystems, HAL, and Driver implementations across all supported architectures (x86_64, arm64, riscv64, shakti).

## 1. Executive Summary

This document provides a detailed gap analysis of the Bharat-OS Hardware Abstraction Layer (HAL) and Device Drivers layer. The analysis focuses on how the OS communicates with the underlying physical architecture—specifically examining interrupt handling, synchronization (multi-core and locks), memory-mapped I/O (MMIO), IOMMU translation, context switching efficiency, and hardware accelerator support.

Overall, while the architecture cleanly separates mechanisms (in the kernel) from policies (in user-space/subsystems) via capabilities, the current implementation heavily relies on stubs, simplifications, and mock hardware. To achieve production readiness, specifically for Edge and Datacenter profiles, substantial work is needed to replace these stubs with robust, scalable implementations.

### Code-vs-Document Validation Update (2026-03)

The original analysis remains directionally correct, but repository state has moved since the first draft:

- **Partially done:** 32-bit architecture scaffolding now exists in tree (ARM32 and RV32 directories, HAL translation backends, and arch-cap profile hooks).
- **Still not implemented:** most ARM32/RV32 execution paths are placeholders and not wired end-to-end in build/runtime flows.
- **Action needed:** stabilize the core architecture-neutral layer so new 32-bit backends can plug in without changing generic scheduler/memory/security code.

The sections below keep the gap framing and add concrete implementation deltas for ARM32/RV32 readiness.

---

## 2. Interrupt Handling and Delivery

### Current State
- **Generic Abstraction:** `kernel/src/hal/interrupt_common.c` implements a very basic, flat array-based IRQ registry (`HAL_MAX_IRQS = 256U`). It maps one handler per vector and simply tracks dispatch counts.
- **Architecture Implementations:**
  - **x86_64:** Basic LAPIC/IOAPIC support exists (`kernel/src/hal/x86_64/apic.c` and `hal_cpu.c`), but IRQ routing assumes a standard 24-pin IOAPIC and heavily relies on simplistic fixed mappings.
  - **ARM64:** Uses a basic GICv3 abstraction (`kernel/src/hal/arm64/gicv3.c`). It initializes Distributor and Redistributors, but MSI/LPI support (GIC ITS) is explicitly marked as a "simplistic stub allocating LPIs".
  - **RISC-V:** Uses a very simple PLIC stub (`kernel/src/hal/riscv/plic.c`) mapped to QEMU virt offsets, assuming S-mode context blindly. It sets thresholds to 0 unconditionally.


### Identified Gaps
- **Lack of Nested and Shared Interrupts:** The generic layer does not support shared IRQ lines (common in PCI/PCIe) or nested interrupt handling priorities.
- **MSI/MSI-X Support:** Modern high-performance devices rely on Message Signaled Interrupts (MSI/MSI-X). The current HAL has only a placeholder for ARM GIC-ITS and lacks x86_64 MSI support entirely.
- **IRQ Load Balancing:** Multi-core setups do not dynamically distribute IRQs across cores (SMP affinity). Static routing is hardcoded.
- **Top-half / Bottom-half Split:** There is no established software mechanism (like tasklets or softirqs) for deferring heavy interrupt work, which will lead to long interrupt latency and missed deadlines in RTOS profiles.

---

## 3. Memory Management, IOMMU, and DMA

### Current State
- **IOMMU Subsystem:** The generic IOMMU interface (`kernel/src/hal/iommu_stub.c`) is completely stubbed (`hal_iommu_init` returns `-1`, etc.).
- **Architecture Implementations:**
  - **x86_64 VT-d:** `kernel/src/hal/x86_64/vtd_iommu.c` contains a framework, but explicitly notes: "(Hardware programming omitted for stub)".
  - **ARM64 SMMU:** `kernel/src/hal/arm64/smmu.c` implements the struct wrapper but lacks the low-level translation table programming.
  - **RISC-V IOPMP:** `kernel/src/hal/riscv/iopmp.c` exists but relies on simple block device mocks.
- **MMU and Page Tables:** The basic multi-level page tables exist for all architectures (e.g., `hal_pt_arm64.c`, `hal_pt_riscv64.c`), but TLB shootdowns are rudimentary. Shootdown loops (`hal_ipi_broadcast`) blindly iterate masks without sophisticated generation tracking or backoff.

### Identified Gaps
- **DMA Security:** Without a functioning IOMMU, unprivileged user-space drivers (a core tenet of the Bharat-OS architecture) could bypass capability isolation by programming a device to DMA over kernel memory.
- **Cache Coherency:** There is no explicit HAL API for managing cache coherency for non-coherent DMA devices (flushing/invalidating caches around DMA boundaries). This will cause data corruption on embedded ARM/RISC-V profiles.

---

## 4. Synchronization and Multi-Core Coherency

### Current State
- **Spinlocks:** The primary locking mechanism used (e.g., in `kernel/src/console/console.c`) is a naïve `__sync_lock_test_and_set` loop (`while(*lock);`).
- **Atomics:** Atomic operations rely exclusively on compiler built-ins (`__sync` rather than modern `__atomic` C11 standard builtins).
- **Core Bring-up:** `kernel/src/multicore.c` brings up secondary cores by broadcasting SBI IPIs on RISC-V. For x86_64 and ARM64, the phase 1 SMP init merely maps URPC channels and assumes the CPU is ready.

### Identified Gaps
- **Lock Contention:** Naïve spinlocks lack backoff algorithms (like exponential backoff or ticket locks) or architecture-specific yield hints (e.g., `PAUSE` on x86, `YIELD` on ARM). Under high core counts, this will cause severe memory bus saturation and priority inversion.
- **Lockless Structures:** While URPC is designed to be lockless, internal kernel structures (like the async IPC request array in `kernel/src/ipc/async_ipc.c`) currently use a simple global array iteration without robust concurrency controls, presenting a bottleneck for high-throughput messaging.

---

## 5. Context Switching and Register State

### Current State
- **Thread Context:** The `arch_prepare_initial_context` logic (`kernel/src/arch/*/context_switch.c`) cleanly sets up stack boundaries and initial registers (e.g., aligning SP to 16 bytes per ABI, setting up trampoline return addresses).
- **Floating Point / Vector state:**
  - ARM64 mentions FPU/SIMD enabling in `CPACR_EL1` during boot, but dynamic lazy save/restore of heavy FPU/NEON registers during context switches is missing or severely unoptimized.
  - RISC-V explicitly sets the `FS` bits to `Initial` to enable FPU instructions, but the full context preservation logic lacks advanced vector extension (RVV) support.

### Identified Gaps
- **Optimized Context Saving:** The kernel always saves and restores the full general-purpose register set on every trap/switch. Differentiating between synchronous system calls (where clobbered registers don't need saving) and asynchronous IRQs can save hundreds of cycles per switch.
- **Lazy FPU State:** Saving extended states (FPU/SIMD/AVX) unconditionally on every context switch introduces immense latency. A lazy save/restore mechanism utilizing trap-on-first-use is required for performance.

---

## 6. Hardware Accelerators and Specialized Subsystems

### Current State
- **NPU (Neural Processing Unit):** `kernel/src/hal/npu.c` provides a capability-gated API for allocating and enumerating NPUs. However, the implementation is purely a mock (`vendor_id = 0xABCD`, hardcoded memory sizes) and doesn't map real MMIO or IRQs.
- **FPGA Manager:** `drivers/accel/fpga_mgr.c` is an empty shell with a single function `fpga_mgr_load_bitstream` that returns 0 without touching hardware.
- **Performance Monitor Units (PMU):** PMU implementations across architectures (`kernel/src/hal/x86_64/pmu.c`, `arm64/pmu.c`, `riscv/pmu.c`) are marked as "Simple abstraction stubs" and often return approximate cycle counts (e.g., using `rdtsc` directly in x86 instead of full PMC event programming due to QEMU `#GP` concerns).

### Identified Gaps
- **Real HW integration:** Support for complex heterogeneous accelerators (like GPUs, NPUs, FPGAs) requires functional IOMMU support, shared virtual memory (SVM) capabilities, and robust DMA scatter-gather lists—all of which are currently missing.
- **AI Scheduler Telemetry:** The AI scheduler relies on the PMU for cycles/instructions telemetry. With PMUs stubbed or using simple `rdtsc`, the scheduler receives low-fidelity data, neutering the planned "AI predictive heuristics".

---

## 7. Runtime Hardware & Feature Probing

### Current State
- **CPU Capabilities:** The files responsible for probing CPU features at runtime (`kernel/src/arch/*/cpu_caps.c`) across all architectures (x86_64, arm64, riscv64, shakti) contain empty stubs: `// Stub: Probe CPUID for x87, SSE, AVX, etc.`.
- **System Discovery:** The `hal_discovery.c` structure exists to parse ACPI/FDT, but it only maps basic interrupt controllers and memory bounds. It does not actively parse or expose custom extensions (like RISC-V `V` vector extension, ARM SVE, or Intel AVX-512) to the kernel or user-space services.

### Identified Gaps
- **Lack of Accelerator Discovery:** User-space subsystems and drivers cannot dynamically leverage hardware extensions or accelerators because the kernel does not probe or expose these capabilities.
- **Static Assumption:** The system currently assumes a static baseline instruction set per build target, leading to unoptimized code on newer hardware and potential illegal instruction faults if features are used blindly without runtime probing.
- **Internal Kernel Optimization:** The kernel itself fails to take advantage of advanced ISA extensions (e.g., using ARM NEON, RISC-V Vector `V` extension, or Intel AVX for fast page clearing, memory copying, or cryptographic hashing). It lacks the infrastructure to conditionally dispatch to optimized ASM routines or macros when these features are detected at runtime.

---

## 8. Hardware-Backed Security and Secure Boot

### Current State
- **Encryption & Secure Boot:** The HAL entirely lacks integration with hardware roots of trust (e.g., TPMs, ARM TrustZone, Intel SGX/TDX, RISC-V Keystone).
- **Cryptographic Accelerators:** While the architectural roadmap mentions isolated crypto services, the HAL does not provide memory-mapped windows or DMA pathways specifically secured for cryptographic engines. The `fpga_mgr.c` stub mentions "secure boot requirement" in comments but lacks any implementation.

### Identified Gaps
- **Missing Chain of Trust:** There is no HAL interface to verify bootloader-passed measurements or to restrict memory regions to secure enclaves.
- **Key Management:** The kernel cannot securely lock memory or isolate cryptographic keys from the rest of the OS because secure memory primitives (like zero-on-free or TrustZone SMC calls) are missing from the HAL.

### Immediate Interface Additions (2026-03)
- Added common HAL contracts for:
  - boot measurement retrieval and root-of-trust verification (`hal_secure_boot_get_measurements`, `hal_secure_boot_verify_measurements`);
  - secure memory-region restriction/release (`hal_secure_mem_restrict_region`, `hal_secure_mem_release_region`);
  - hardened crypto accelerator MMIO+DMA windows (`hal_secure_crypto_dma_window_config`, `hal_secure_crypto_dma_window_clear`).
- Added kernel wrappers to lock/unlock key-bearing memory regions (`bharat_secure_key_region_lock`, `bharat_secure_key_region_unlock`).
- `boot_trust_verify_evidence()` now attempts to refresh unknown trust evidence from HAL measurements before applying strict policy checks.

> Note: these APIs are foundational and currently default to weak fallback stubs (`-1`) until architecture/board backends are implemented.

---

## 9. 32-bit Architecture Support (ARM32 / RV32)

### Current State
- **ARM32 now has explicit scaffolding:** `kernel/src/arch/arm32/*` and `kernel/src/hal/arm32/*` exist, including context-switch placeholders and an MMU-Lite translation backend (`hal_pt_arm32.c`).
- **RV32 now has explicit scaffolding:** `kernel/src/hal/riscv32/hal_pt_riscv32.c` and `kernel/src/arch/riscv32/arch_caps.c` exist.
- **Build wiring remains inconsistent:** `ARCH=riscv32` and `ARCH=arm32` CMake selections still reference several 64-bit sources (e.g., riscv64/arm64 boot and HAL files, shakti context for riscv32), indicating transitional rather than native 32-bit pipelines.

### Identified Gaps
- **Scaffold-only state:** ARM32/RV32 page-table backends return `ENOSYS` for map/unmap/protect/query paths, and key translation helpers (`phys_to_virt`, `virt_to_phys`) are placeholders.
- **Context-switch incompleteness:** ARM32 context switch C/ASM stubs are non-functional; RV32 lacks dedicated context-switch implementation files.
- **Security and isolation parity gap:** 32-bit paths do not yet have equivalent secure-boot/HAL security backend implementations beyond unsupported defaults.
- **Embedded RTOS profile limitations:** MPU-only and MMU-Lite abstractions exist conceptually, but not enough concrete backend behavior exists to guarantee isolation or deterministic RT behavior on Cortex-M/RV32-class systems.

### Done vs Not Implemented (Focused Snapshot)

**Done (foundation in place):**
1. Architecture-neutral translation contracts (`hal_pt`, `hal_translate_ops`, execution classes MMU_FULL/MMU_LITE/MPU_ONLY).
2. ARM32 and RV32 specific HAL PT backend source files added.
3. ARM32 and RV32 architecture capability/profile entry points exist.
4. Generic spin-wait hint now includes x86 pause / ARM yield / RISC-V nop path hooks.

**Not implemented (blocking production support):**
1. Real ARM32 page-table or MPU programming (backend currently stubbed).
2. Real RV32 Sv32 page-table programming (backend currently stubbed).
3. Native ARM32 and RV32 boot + trap + context-switch + timer + interrupt path integration in CMake/runtime.
4. 32-bit-safe DMA/IOMMU and security-backed memory isolation.
5. Runtime CPU feature probing for ARM32/RV32 (current probes are placeholder-level).

### Re-analysis Notes Against Latest Code (2026-03)

**What is concretely done in code now (verified):**
- `hal_pt.c` already has architecture dispatch branches for `__arm__` and `__riscv_xlen == 32`, so the generic translation entry point is prepared to select ARM32/RV32 backends.
- ARM32 and RV32 each provide dedicated HAL PT backend files (`hal_pt_arm32.c`, `hal_pt_riscv32.c`) and expose `hal_translate_ops()` symbols under the expected compile guards.
- ARM32 and RV32 architecture capability files exist (`arch/arm32/arch_caps.c`, `arch/riscv32/arch_caps.c`) so capability-gated policy integration has a starting hook.

**What is still not implemented in code (verified):**
- ARM32/RV32 PT backends are scaffold-only: create/map/unmap/protect/query paths still return fixed stub values (`0` or `-1`) rather than programming MMU tables.
- ARM32 context switch is currently non-functional (`context_switch()` empty; assembly symbol loops forever via branch-to-self), and RV32 has no dedicated context-switch pair.
- Build plumbing for 32-bit targets is transitional: CMake still routes `ARCH=arm32` through arm64 boot/HAL/context files and `ARCH=riscv32` through riscv64 boot/HAL plus shakti context sources.
- The shared atomic layer still centers on legacy `__sync` intrinsics with unconditional 64-bit atomic helpers; this is risky for strict 32-bit portability/performance without capability-aware fallback paths.

### Required Sequencing for ARM32/RV32 Bring-up (to keep core stable)

1. **Build-graph correctness first**
   - Create native 32-bit source sets in CMake (boot, trap, context-switch, timer, irq, dma) so CI validates the intended architecture path rather than mixed 64-bit proxies.
2. **Context and trap safety second**
   - Implement minimal but correct ARM32 and RV32 context switch + trap entry/exit before any scheduler/perf tuning.
3. **MMU/MPU backend completeness third**
   - Implement map/unmap/protect/query + TLB maintenance semantics in `hal_pt_*32.c` so generic VM code can run unchanged.
4. **DMA/IOMMU security parity fourth**
   - Provide explicit 32-bit backend behavior for non-coherent DMA and iommu/no-iommu fallback policy to preserve isolation guarantees.
5. **Atomic + capability hardening fifth**
   - Migrate shared atomics/locks to `__atomic` wrappers and gate 64-bit operations via arch capabilities to avoid latent portability traps.
6. **Performance tuning last**
   - Only after correctness: add IRQ affinity, lazy ext-state handling, and scheduler fast paths.

### Core-Code Changes Required So ARM32/RV32 Can Land Without Reworking Core Again

1. **Make address-size assumptions explicit and centralized**
   - Introduce compile-time contracts (`ARCH_WORD_BITS`, `ARCH_PADDR_BITS`, `ARCH_VADDR_BITS`) and assert these in shared headers.
   - Audit generic code for implicit `uint64_t` assumptions in mapping, bitmap math, and pointer casts.

2. **Split generic VM logic from page-table format details**
   - Keep `hal_pt` as the only mapping gateway from generic MM/VMM code.
   - Forbid generic code from directly depending on level counts (e.g., 4-level assumptions) or 64-bit PTE layouts.
   - Provide common helpers for page-size-agnostic alignment/rounding to avoid duplicated backend math.

3. **Harden atomic and lock abstraction for 32-bit**
   - Migrate from legacy `__sync` builtins to `__atomic` wrappers with explicit memory orders.
   - Gate 64-bit atomics behind capability checks or fallback lock paths when native 64-bit atomic ops are not guaranteed on 32-bit targets.

4. **Define strict per-arch context ABI contract**
   - Standardize `arch_context_t` save/restore metadata for GPR, status register, and optional ext-state.
   - Make lazy ext-state save/restore opt-in via arch capability bits to avoid forcing 64-bit assumptions into 32-bit minimal profiles.

5. **Introduce architecture capability-gated fast paths**
   - Ensure scheduler, IPC, and MM paths select behavior through `arch_caps` rather than compile-time architecture name branching.
   - Add explicit fallbacks for "no IOMMU", "MPU-only", and "non-coherent DMA" scenarios.

6. **Fix build matrix to test what we claim to support**
   - Add CI targets that compile and link `ARCH=arm32` and `ARCH=riscv32` with their own boot/HAL/context sets.
   - Promote a "stub budget" check: fail CI if critical arch interfaces remain unresolved or intentionally stubbed beyond allowed exceptions.

---

## 10. Kernel Boundaries and Subsystem API

### Current State
- **Mechanism vs Policy:** The architecture aims to separate mechanisms (in the kernel) from policies (in user-space). The system call API (e.g., `trap.c`) dispatches requests like `SYSCALL_ENDPOINT_RECEIVE` or `SYSCALL_CAPABILITY_DELEGATE` based on capability tables.
- **Pointer Validation:** Trap handlers make rudimentary checks (`trap_user_ptr_valid`), but advanced copy-in/copy-out mechanisms robust against time-of-check to time-of-use (TOCTOU) attacks are not fully fleshed out.
- **Module Isolation:** Modules and subsystems are heavily defined via CMake stubs but practically lack isolated runtime boundaries on hardware lacking full MPU/MMU enforcement.

### Identified Gaps
- **Weak Boundary Enforcement:** On smaller profiles (EDGE/RTOS), if the IOMMU or MPU wrappers (like the ARM32 mock) are non-functional, a buggy user-space driver or subsystem can trivially panic the entire kernel or corrupt physical memory.
- **ABI Instability:** The raw system call and BIDL interface lack a robust versioning mechanism, making independent module updates fragile.

---

## 11. Core OS Primitives: Scheduler and Timers

### Current State
- **Scheduler (`sched.c`):** The kernel implements a basic tick-driven scheduler with strict Priority scheduling (`SCHED_MAX_PRIORITY`). It includes a rudimentary global array iteration for load balancing (`sched_balance_once`).
- **Timers (`hal_timer_isr`):** Relies on a single, fixed-frequency tick (e.g., APIC Timer on x86, PLIC/SBI on RISC-V).
- **Async CPU Primitives:** `async_ipc.c` manages async requests, but relies on a global lockless-but-linear-scan array.

### Identified Gaps
- **O(N) Scheduler Complexity:** The scheduler uses basic lists and array scans. It lacks O(1) bitmapped runqueues or Red-Black tree structures (like CFS), which will scale poorly with many threads.
- **Tickless / High-Res Timers:** There is no tickless kernel (NO_HZ) implementation. The constant periodic tick wastes power and degrades real-time determinism.
- **SMP Scaling:** The load balancer is simplistic and invoked infrequently (`g_sched_ticks % 16U`). Global structures for async IPC and scheduling queues lead to heavy cache-line bouncing.

---

## 12. TCB, Threads, Processes, and Per-Core State

### Current State
- **Thread Control Block (TCB):** `kthread_t` tracks state, priority, capabilities, and CPU affinity. `kprocess_t` maps to an `address_space_t`.
- **Per-Core State (`cpu_local.c`):** Uses hardware registers (e.g., `tpidr_el1` on ARM64, `tp` on RISC-V) to quickly resolve `cpu_local_t`, pointing to the current thread and address space.
- **Kernel Debugging (`fault_diag.c` / `panic.c`):** Implements `fault_breadcrumbs_t` to track the last syscall and fault VA per core. Emits detailed panic headers containing register state and breadcrumbs.

### Identified Gaps
- **Memory Management (VMM/PMM):** Demand paging and Copy-on-Write (COW) exist as API scaffolds (`vmm_handle_cow_fault`), but heavily rely on the missing architectural MMU implementations. Physical page tracking does not aggressively handle defragmentation or NUMA node awareness beyond preferred hints.
- **Trap Handling Depth:** The unified trap handler (`trap.c`) handles fatal bounds (e.g. guard page hits causing stack overflow), but recovery or user-space signal translation (e.g. mapping to `SIGSEGV` for the Linux personality) is marked as a `TODO`.
- **Debugging Limits:** While `panic.c` prints register dumps, there is no interactive kernel debugger (KGDB stub), DTrace/eBPF-like dynamic tracing, or crash dump (kdump) facility. Debugging complex multikernel state transitions remains a purely print-based exercise.

---

## 13. Conclusion and Recommendations

Bharat-OS has successfully laid down the architectural scaffolding for a multikernel, capability-based system. The mechanism-vs-policy boundaries are respected, and the abstractions (HAL interfaces) exist in the right places.

However, moving from a "bootable baseline" to a high-performance OS requires systematically removing the stubs identified in this report.

**Immediate Priorities for Improvement:**
1. **Implement IOMMU & DMA protections:** Crucial for the security model of user-space drivers.
2. **Upgrade Synchronization Primitives:** Replace simple spinlocks with ticket/qspinlock variants and integrate architecture yield instructions.
3. **Refine Interrupts:** Introduce MSI/MSI-X support and IRQ load balancing.
4. **Optimize Context Switches:** Implement lazy FPU/Vector state saving to drastically reduce IPC and scheduling latency.
5. **Implement Runtime Probing:** Populate `cpu_caps.c` to parse CPUID/FDT and expose hardware features to user-space services.
6. **Establish Hardware Security Foundations:** Introduce HAL interfaces for TPM/TrustZone interaction to support the secure boot roadmap.
