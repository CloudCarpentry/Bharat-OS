---
status: accepted
date: 2025-03-25
owner: Divyang Panchasara
---

# ADR-014: Library Layering and Kernel-Private Data Structures

## Context and Problem Statement
Bharat-OS historically intermingled reusable library functions with kernel-specific mechanisms, resulting in unclear boundaries. Additionally, headers for kernel-private data structures (e.g., Cuckoo Hash, Radix Tree, uRPC Rings) were placed in `lib/include/ds/`, which is intended to be a shared user-space SDK/library surface. This created a misleading public API boundary, exposing headers that user-space could see but not link against, and complicating the separation of kernel state from portable code.

## Decision Drivers
* **Strict Layer Separation:** `lib/` must remain reusable and free from kernel dependencies, while `kernel/src/lib/` may use kernel-specific features (allocators, locking, traps).
* **SDK Integrity:** Headers placed in `lib/include/` imply a reusable shared ABI surface. Kernel-only features must not pollute this namespace.
* **Profile-Driven Enablement:** The build system must support profile stripping. Giant monolithic libraries should be avoided in favor of narrow feature flags and independent modules.
* **Hardware Acceleration Strategy:** Provide a generic portable fallback (in `lib/` or `hal/common/`), overridden by architecture-specific hooks (`arch/`, `hal/`) behind stable interfaces.

## Decision
We enforce a strict 3-layer library model:

1. **`kernel/src/lib/` & `kernel/include/lib/`**: Strictly kernel-internal helpers (e.g., Radix Tree, MCS locks, uRPC rings). These implementations use kernel allocators/state, and their headers **must** reside in `kernel/include/lib/`, not `lib/include/`.
2. **`lib/` & `lib/include/`**: Reusable user-space/shared library code (e.g., portable string/memory functions). Zero kernel dependency.
3. **`hal/` & `arch/`**: Hardware/ISA optimized implementations (e.g., `arch_memcpy`) with portable fallbacks in `hal/common/`.

As part of this decision:
* Data structure headers previously in `lib/include/ds/` and `lib/include/sync/` (which lack portable shared implementations) are relocated to `kernel/include/lib/ds/` and `kernel/include/lib/sync/`.
* The `lib/string` library is fully separated from `kernel/src/lib/string.c`, allowing the kernel to leverage its internal dispatch (and ISA accelerations) while user-space gets a pure C portable implementation.

## Consequences
### Positive
* **SDK Clarity:** User space only sees APIs it can actually link against and use.
* **Kernel Isolation:** Kernel data structures can safely rely on internal allocators (`kalloc`) without risking accidental linkage from user space.
* **Scalability:** The architecture correctly supports small-profile builds by enabling independent, modular feature flags.

### Negative
* If dual-track implementations are required (i.e., a user-space Radix tree *and* a kernel-space Radix tree), they will need distinct implementations (one in `lib/ds/` and one in `kernel/src/lib/ds/`), increasing maintenance overhead slightly.
