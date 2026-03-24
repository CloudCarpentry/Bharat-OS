# Bharat-OS Memory Debugging Strategy: Valgrind, Sanitizers, and Native Hardening

This document outlines the strategic plan for ensuring memory correctness within Bharat-OS. It defines a multi-layered approach that strictly separates host-testable logic from the freestanding, bare-metal kernel, ensuring each tool is used where it provides the highest value.

## 1. Executive Summary

Valgrind is highly valuable for host-side development (unit tests, allocators, parsers) but is structurally unsuited for validating the actual freestanding multikernel image executing on bare metal or QEMU. Trying to force Valgrind to run the kernel directly often devolves into a science project.

Instead, Bharat-OS employs a **three-layer memory debugging model**:
1.  **Host-process validation with Valgrind**: For standalone libraries, allocators, parsers, and services.
2.  **Compiler sanitizers (ASan/UBSan/LSan/TSan)**: For rapid local and CI validation of host binaries.
3.  **Kernel-native memory hardening (`BHARAT_MM_DEBUG`)**: For physical page tracking, ownership proofs, and fault injection in the real OS.

## 2. Where Valgrind and Sanitizers Help Most

These tools are explicitly reserved for components that can be natively compiled and executed on a host OS (Linux/macOS):

*   Host unit tests for PMM/VMM helper logic (`kernel/src/tests/`, `tests/host/`).
*   Memory allocator logic compiled as native Linux executables (e.g., bitmap/freelist logic).
*   Contract parsers, IPC serializers, ELF loaders, config parsers.
*   User-space services in the Bharat-OS repo (managers, daemons, runtime helpers).
*   Fuzz harnesses for kernel-adjacent libraries.

**Valgrind should NOT be used for:**
*   Bare-metal kernel running under its own page tables.
*   Interrupt context bugs or lockless SMP race validation.
*   MMU/TLB shootdown correctness or physical page ownership bugs.

## 3. The Three-Layer Rollout Plan

### Phase 1: Immediate Value via Host CI Lanes (Valgrind + Sanitizers)

**Target Scope:** `tests/host/`, service binaries, shared runtime libraries, config/parsing/IPC/ELF code.

*   **Sanitizer Lane (`host-test-asan`)**: Build host tests with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) enabled. This finds bugs extremely quickly during normal development and CI.
*   **Valgrind Lane (`host-test-valgrind`)**: Execute host tests through Valgrind to catch leaks, double-frees, and use-after-free conditions that sanitizers might miss in complex teardown paths.
    *   Typical command pattern: `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./test_binary`

### Phase 2: Refactor Kernel Logic into Testable Host Libraries

To maximize the value of Phase 1, purely logical components of the kernel should be decoupled from hardware state so they can run as host binaries.

*   **Targets for Refactoring:**
    *   VM Region trees and interval structures (`kernel/src/mm/vm/aspace/`).
    *   VM Object lifecycle helpers and refcounting.
    *   Capability table pure-logic checks (`kernel/src/cap/`).
    *   Scheduler math helpers.
    *   Page accounting and bitmap state transitions.

### Phase 3: Build Kernel Debug Memory Mode (`BHARAT_MM_DEBUG`)

For the actual kernel image running in QEMU or on hardware, native memory hardening is required. The `BHARAT_MM_DEBUG` CMake option enables these features:

*   **PMM/Heap Hardening:**
    *   Page state enum validation.
    *   Owner tag per physical page (verifying single-core ownership).
    *   Page poisoning on free (e.g., writing `0xDEADBEEF`).
    *   Allocation canaries / redzones around slab objects.
    *   Verification that no page exists on multiple freelists simultaneously.
*   **VMM Hardening:**
    *   Map/unmap invariants and per-aspace accounting.
    *   TLB invalidation assertions and page table page leak checks.

### Phase 4: Long-Term Vision (KASAN-like Kernel Instrumentation)

Once the core debug mode is stable, a serious kernel path will pursue a KASAN-style shadow-memory model rather than stretching Valgrind into kernel territory.

## 4. Recommended CI Stack

The integrated pipeline consists of:
1.  **Fast Lane**: Native ASan/UBSan/LSan build + tests (`host-test-asan`).
2.  **Deep Lane**: Native host build run through Valgrind (`ctest -T memcheck` or script).
3.  **Integration Lane**: QEMU debug builds (`BHARAT_MM_DEBUG=ON`) for boot/runtime kernel checks, verifying page poisoning and ownership tracking.

## 5. Summary

*Adopt Valgrind — but only for host-testable kernel logic, shared libraries, and user-space services. For the real kernel, invest in native memory-debug infrastructure instead.*
