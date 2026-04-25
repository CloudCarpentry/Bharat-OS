# Bharat-OS Library Layering and Migration Roadmap

## Current State Findings

Bharat-OS historically intermingled reusable library functions with kernel-specific mechanisms, resulting in unclear boundaries. A key symptom of this ambiguity is the presence of generic helpers within `core/kernel/src/lib/` or internal memory helpers that lack strict layer definition and abstraction.

This roadmap outlines the plan to strictly enforce the boundaries defined in `docs/architecture/shared/lib_architecture.md`.

## Phased Migration Steps

### Phase 1: Boundary Definition and Initial Enforcement (Immediate)

1. **Establish Canonical Locations:**
   * Create boundary `README.md` files in `lib/` and `core/kernel/src/lib/` explaining the core policy.
   * Formalize the rule: `lib/` must not depend on `core/kernel/`.
   * **Header Isolation:** Relocate any headers intended for kernel-only implementations from `lib/include/` into a kernel-private include layer such as `core/kernel/include/lib/` or `core/kernel/include/ds/`. Only keep a header under `lib/include/` if it is supported as a shared/public library contract.
2. **Build System Restrictions:**
   * Introduce CMake rules (e.g., in `lib/CMakeLists.txt` or a common policy file) that strictly prevent target linkage from `lib/` back to kernel targets.
   * **Status (2026-04-22):** Implemented a configure-time enforcement pass in `lib/CMakeLists.txt` that fails configuration if any `lib/` target links kernel-private targets.
3. **Identify Misplaced Code:**
   * Audit `core/kernel/src/lib/` (e.g., `rbtree.c`, `string.c`, `status.c`) and migrate purely logical, zero-kernel-state data structures (like standard linked lists or rbtrees, if allocation-free) to `lib/`.
   * Conversely, move any helper currently in `lib/` that relies on kernel locking, traps, or physical memory allocation directly into `core/kernel/src/lib/`.

### Phase 2: Hardware-Accelerated Dispatch Architecture (Near Term)

1. **Standardize Memory Operations:**
   * Introduce a unified header/interface for `memcpy`, `memset`, and `memmove`.
   * Implement a fallback portable C path in `lib/string/`.
2. **Architecture Hooks:**
   * Introduce early-boot feature detection (e.g., AVX, NEON) within the HAL.
   * Add function pointers or alternate dispatch methods for architecture-optimized string functions, overriding the fallback path where available.
3. **Formalize DMA Offload Strategy:**
   * Establish explicit contracts for DMA memory copies (e.g., bulk zeroing). This should explicitly bypass small/inline memory utilities and be tied to specific VMM or stack APIs.

### Phase 3: Contract/BIDL Structuring (Mid Term)

1. **Define Source of Truth:**
   * Relocate or formalize all BIDL interface definitions into a top-level `contracts/` directory.
2. **Runtime Support Separation:**
   * Build the BIDL runtime (encode/decode, bounds checking) as a standalone shared library in `lib/bidl_runtime/`.
   * Update the IPC stack and user services to link against `lib/bidl_runtime/` rather than re-implementing parsing logic.

### Phase 4: Tier 1 Data Structure Standardization (Long Term)

1. **Provide Core Containers:**
   * Standardize the implementation of intrusive lists, ring buffers, bitmaps, and ID allocators in `lib/`.
   * Ensure these implementations accept external allocators or operate purely on pre-allocated contiguous memory to enforce zero-kernel-state guarantees.
2. **Transition Kernel Subsystems:**
   * Migrate existing ad-hoc kernel queues and resource trackers to use the standardized `lib/` structures.

### Phase 5: Multikernel Algorithms and Advanced Data Structures (Short-to-Mid Term)

1. **Replace Global Locks:**
   * Introduce MCS locks or hazard pointers for critical sections.
   * Implement per-core ring buffers for uRPC.
2. **Optimize Thread Lookup:**
   * Transition to cuckoo hashing utilizing hardware CRC32 acceleration.
3. **Memory Locality:**
   * Integrate per-core PMM caches (magazine allocator) and NUMA-aware allocation strategies.

### Phase 6: Next-Generation Hardware Integration (Long Term)

1. **Lock-Free Capability Tables:**
   * Migrate capability delegation structures to use RCU combined with radix trees.
2. **Hardware-Accelerated Cryptography:**
   * Establish interfaces to offload cryptography to AES-NI (x86) or Crypto Extensions (ARM).
3. **AI-Driven Scheduling:**
   * Incorporate dynamic scheduler hints using performance counters (PMC/PMU).
   * Evaluate offloading complex scheduling decisions to available NPU/TPUs.

## Build and Testing Strategy

* **Host-Testing:** Everything in `lib/` must be compiled and tested on the host (e.g., `cmake --preset host-test`). Any dependency on kernel headers or target hardware limits this ability and is considered a violation.
* **Current Coverage Update (2026-04-22):** Added dedicated host test `host_test_lib_string` for the fallback string/memory path in `lib/string/string.c`, including overlap-sensitive `memmove` behavior.
* **Scalability Testing:** Stress-test lock-free and parallel structures with high core counts (e.g., 64+ cores) to validate scalability and hardware leveraging.
* **Formal Methods:** As the lock-free data structures stabilize (especially MPMC rings and RCU), utilize formal verification (e.g., Isabelle/HOL) to prove their correctness.
* **Continuous Enforcement:** Add pre-commit static analysis or CMake assertion checks to verify dependency directions.
