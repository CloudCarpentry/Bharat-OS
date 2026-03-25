# Bharat-OS Library Layering Architecture

## Purpose and Principles

Bharat-OS strictly separates its library ecosystems into layers to maintain a secure, robust, and multikernel-ready architecture. The boundary between generic reusable libraries, kernel-private implementations, and cross-boundary contracts is strictly enforced both architecturally and within the build system.

The core principle:
> `lib/` is for reusable libraries with controlled dependencies.
> `kernel/src/lib/` is for kernel implementation support.

### 1. Root `lib/` (Shared, Reusable Library Space)

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
