# Bharat-OS Library Layering and Migration Roadmap

## Current State Findings

Bharat-OS historically intermingled reusable library functions with kernel-specific mechanisms, resulting in unclear boundaries. A key symptom of this ambiguity is the presence of generic helpers within `kernel/src/lib/` or internal memory helpers that lack strict layer definition and abstraction.

This roadmap outlines the plan to strictly enforce the boundaries defined in `docs/architecture/shared/lib_architecture.md`.

## Phased Migration Steps

### Phase 1: Boundary Definition and Initial Enforcement (Immediate)

1. **Establish Canonical Locations:**
   * Create boundary `README.md` files in `lib/` and `kernel/src/lib/` explaining the core policy.
   * Formalize the rule: `lib/` must not depend on `kernel/`.
2. **Build System Restrictions:**
   * Introduce CMake rules (e.g., in `lib/CMakeLists.txt` or a common policy file) that strictly prevent target linkage from `lib/` back to kernel targets.
3. **Identify Misplaced Code:**
   * Audit `kernel/src/lib/` (e.g., `rbtree.c`, `string.c`, `status.c`) and migrate purely logical, zero-kernel-state data structures (like standard linked lists or rbtrees, if allocation-free) to `lib/`.
   * Conversely, move any helper currently in `lib/` that relies on kernel locking, traps, or physical memory allocation directly into `kernel/src/lib/`.

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

## Build and Testing Strategy

* **Host-Testing:** Everything in `lib/` must be compiled and tested on the host (e.g., `cmake --preset host-test`). Any dependency on kernel headers or target hardware limits this ability and is considered a violation.
* **Continuous Enforcement:** Add pre-commit static analysis or CMake assertion checks to verify dependency directions.
