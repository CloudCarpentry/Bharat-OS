# Bharat-OS Library Layering Architecture

## Purpose and Principles

Bharat-OS strictly separates its library ecosystems into layers to maintain a secure, robust, and multikernel-ready architecture. The boundary between generic reusable libraries, kernel-private implementations, and cross-boundary contracts is strictly enforced both architecturally and within the build system.

The core principle:
> `lib/` is for reusable libraries with controlled dependencies.
> `kernel/src/lib/` is for kernel implementation support.

### 1. Hard Split Between Three Layers
* **`kernel/src/lib/`**: Kernel-private helpers. May use kernel allocators (`kalloc`), kernel locking, per-core state, and trap-safe assumptions. Its headers reside in `kernel/include/lib/`.
* **`lib/`**: Reusable user-space/shared library code. Must have **zero** kernel dependency. Provides portable fallback implementations. Its headers reside in `lib/include/`.
* **`hal/` or `arch/`**: Hardware/ISA optimized implementations selected by the build/arch/profile system (e.g., `arch_memcpy`).

### 2. Root `lib/` (Shared, Reusable Library Space)

The top-level `lib/` directory contains platform-agnostic, dependency-disciplined libraries intended for broad reuse.

**Authorized Consumers:**
* User-space runtimes
* System services
* Host tools and test frameworks
* Kernel (only under strict usage guidelines)

**Rules for `lib/` Code:**
* **Zero Kernel State:** Must not touch kernel globals, internal locks, or trap state.
* **No Implicit Privileges:** Cannot assume execution in ring 0, disabled interrupts, or elevated context.
* **Explicit Allocations:** Must not call kernel-internal allocators (`kalloc`, etc.) directly. Memory ownership and allocator functions must be explicitly injected or passed.
* **Platform Agnostic:** Should rely on generic architectural definitions or explicit HAL abstractions, not hardcoded hardware assumptions.
* **Header Placement:** Only keep a header under `lib/include/` if it is supported as a shared/public library contract. If it is only used by the kernel, it must be moved to `kernel/include/`.
* **Build Enforcement:** The `lib/` CMake layer must fail configuration if shared-library targets attempt to link to kernel-private targets.

**Examples of Valid Contents:**
* Generic data structures (e.g., intrusive lists, ring buffers)
* Serialization/deserialization helpers
* Bounded string and memory utility functions
* Byte-order conversion helpers
* BIDL runtime support (encode/decode/bounds checking)

### 2. `kernel/src/lib/` (Kernel-Private Utilities)

The `kernel/src/lib/` directory contains helpers and utilities explicitly bound to the kernel implementation. It is **not** a shared API surface.

**Authorized Consumers:**
* Core kernel subsystems
* Architecture and HAL bridging logic within the kernel
* In-kernel boot and test frameworks

**Rules for `kernel/src/lib/` Code:**
* **Kernel-Private Include:** Kernel-only DS implementations (like Cuckoo hash, Radix Tree, uRPC rings) MUST NOT place their headers in `lib/include/`. They belong in `kernel/include/lib/` to prevent publishing an API that user-space can see but cannot link against.
* **Privileged Execution:** May freely use kernel internals, spinlocks, scheduler state, IRQ masks, and CPU-local contexts.
* **Implicit Allocations:** May directly depend on kernel allocators (`kalloc`, `vm_alloc`).
* **Fault Handling:** May contain fault-safe formatting or panic helpers specific to the kernel domain.

---

## Hardware-Accelerated Memory and String Operations

Bharat-OS defines a layered architecture for critical operations like `memcpy`, `memset`, and string utilities to leverage hardware capabilities securely without compromising maintainability.

### The 4-Layer Dispatch Model

1. **Public API Surface (`lib/string/` or internal contracts):**
   * Exposes canonical `memcpy`, `memset`, `memmove`, and string utility contracts.
   * Defines strict behavior guarantees for overlap, alignment, and ordering.

2. **Generic Implementation (Fallback):**
   * A portable, correctness-first C implementation.
   * Always available and used as the baseline for testing and unsupported architectures.

3. **Arch-Optimized Implementations (ISA-Specific):**
   * Tuned paths leveraging specific ISA extensions (e.g., AVX on x86-64, NEON on ARM64, V-extension on RISC-V).
   * Feature-detected at boot and patched/dispatched via HAL/Arch hooks.

4. **Platform / DMA Offload Layer:**
   * An optional, tightly controlled path utilizing DMA or memory-engine assistance.
   * **Rule:** DMA is *not* a universal memcpy replacement.
   * **When to use:** Only justified for bulk page copies, zeroing large regions, or specific IO pipeline transfers where the size threshold offsets setup latency, memory is DMA-safe/coherent, and the caller context permits asynchronous or blocking wait models.

---

## Advanced Data Structures Strategy

To support the scalability of Bharat-OS as it matures, data structures are prioritized into tiers based on immediate necessity versus specialized requirements.

### Tier 1: Immediate/Common Foundation
Structures critical for basic kernel stability and generic messaging.
* **Intrusive Lists:** Zero-allocation queueing.
* **Ring Buffers:** Essential for URPC, messaging, and telemetry.
* **Bitmaps/Bitsets:** Compact resource tracking (e.g., allocator pages).
* **ID Allocators:** Handle and endpoint generation.
* **Bounded Queues & Fixed-Size Pools:** Deterministic, allocation-free object management.

### Tier 2: Kernel/Subsystem Scale-up
Structures required as the system scales concurrently and handles massive namespaces.
* **Radix Trees / Interval Trees:** Managing memory maps, VFS ranges, and device memory regions.
* **Scalable Object Maps:** Managing capability nodes, service namespaces, and handles securely.
* **Per-Core Queues:** Core-local scheduler runqueues and IRQ worklists to minimize cross-core lock contention.
* **Deadline Queues / Timer Wheels:** High-performance timeout tracking.

### Tier 3: Specialized / Profile-Driven
Structures introduced only when justified by specific profiles (e.g., HPC, high-throughput network).
* NUMA-aware allocation structures.
* DMA scatter-gather indexing helpers.
* Specialized revocation trees for the capability system.
* Lock-free single-producer/single-consumer channels.

---

## Multikernel Data Structures and Algorithms

For a true multikernel architecture like Bharat-OS—where per-core autonomy, message-passing, and hardware acceleration are critical—choosing the right data structures and algorithms is essential. The following strategies ensure efficiency, scalability, and maximal leverage of hardware features.

### 1. Data Structures for Multikernel Efficiency

#### A. Per-Core Data Structures
* **Lock-Free Queues (uRPC/IPC):**
  * *MPMC Ring Buffer:* Cache-aligned, lock-free, and scalable for per-core queues. Leverages CAS or LL/SC for atomic operations and SIMD for batching small messages.
  * *Wait-Free Queues:* Guaranteed progress under contention, relying on atomic RMW support.
* **Per-Core Hash Tables (Thread/Process Lookup):**
  * *Cuckoo Hashing:* O(1) lookup with low contention. Leverages hardware CRC32/CRC64 instructions (x86 `CRC32`, ARM `CRC32`, RISC-V `Zbb`).
  * *Hopscotch Hashing:* Lock-free, high concurrency, and low memory overhead.
* **Per-Core Pools (Memory Allocation):**
  * *Slab Allocator:* Reduces fragmentation. Leverages hardware prefetching (x86 `PREFETCH`, ARM `PLD`, RISC-V `PREFETCH`).
  * *Magazine Allocator:* Per-core magazines for lock-free fast paths.

#### B. Global Shared Data Structures
* **Radix Trees (Capability Tables):** Compressed and lock-free (via RCU). Leverages bit manipulation instructions (e.g., x86 `BSF/BSR`, ARM `CLZ`, RISC-V `CTZ`).
* **B-Trees (Filesystem Metadata):** Cache-friendly nodes that fit in cache lines, supporting concurrent lock-free variants.
* **Skip Lists (Priority Schedulers):** Easy lock-free implementation with CAS, efficient for dynamic priorities (e.g., real-time tasks).

### 2. Algorithms for Multikernel Scalability

#### A. Scheduling Algorithms
* **Multilevel Feedback Queue (MLFQ):** Adaptive and balanced. Leverages performance counters (x86 `PMC`, ARM `PMU`, RISC-V `Sstc`) for dynamic adjustments.
* **Earliest Deadline First (EDF):** Optimal for real-time tasks. Leverages precise timer interrupts (x86 `HPET`, ARM `Generic Timer`, RISC-V `CLINT`).
* **Completely Fair Scheduler (CFS):** Proportional fairness using virtual runtime. Leverages hardware timekeeping (`rdtsc`/`rdtscp` or `CNTFRQ_EL0`).

#### B. Synchronization Algorithms
* **MCS Locks (Mutual Exclusion):** Scalable and spins on local variables to reduce cache invalidations. Leverages atomic exchanges (x86 `XCHG`, ARM `LDXR/STXR`).
* **RCU (Read-Copy-Update):** Zero-contention reads for read-heavy workloads. Leverages memory barriers (x86 `MFENCE`, ARM `DMB`, RISC-V `FENCE`).
* **Hazard Pointers:** Lock-free safe memory reclamation avoiding the ABA problem, using atomic pointers.

#### C. Memory Management Algorithms
* **Buddy Allocator (PMM):** O(1) split/merge operations using bitmask operations for free block tracking.
* **Slab Allocator (Kernel Objects):** Cache-friendly object reuse, accelerated via hardware prefetching.
* **NUMA-Aware Allocator:** Allocates memory close to the requesting core using hardware NUMA node IDs (x86 `cpuid`, ARM `MPIDR`, RISC-V `mhartid`).

### 3. Leveraging Hardware Features

* **Atomic Operations:** Lock-free data structures using `CMPXCHG`/`LOCK` (x86), `LDXR/STXR` (ARM), or `AMO*` (RISC-V).
* **SIMD:** Batch processing for uRPC via AVX-512 (x86), NEON/SVE (ARM), or RVV (RISC-V).
* **Virtualization:** Secure capability isolation using VT-x, Virtualization Extensions, or the `H` Extension.
* **Accelerator Integration:**
  * *GPU Offloading:* OpenCL/CUDA for parallel tasks (e.g., cryptography).
  * *DMA Engines:* Zero-copy networking (e.g., virtio DMA).
  * *NPU/TPU:* Offloading AI scheduling hints.

---

## Contracts vs. Libraries (The BIDL Boundary)

**Rule:** BIDL definitions are contracts, not generic libraries.

The architecture explicitly splits Interface Definition Language (BIDL) management into three spaces:

1. **Specification / Architecture Layer:**
   * Located in `docs/architecture/contracts/`.
   * Defines schema structure, wire formats, versioning, and capability transfer rules.

2. **Source-of-Truth Definition Layer:**
   * Located in `contracts/` (e.g., `contracts/bidl/`, `contracts/services/`).
   * Contains the authoritative `.bidl` definition files. No executable code lives here.

3. **Runtime / Helper Layer:**
   * Located in `lib/bidl_runtime/`.
   * Contains the actual executable logic: message builders, bounds checkers, encode/decode routines, and generated stubs.
   * May be used by services and host tools. If the kernel requires raw parsing, specialized fault-safe helpers belong in `kernel/src/lib/bidl_runtime/`.
