# Shared Libraries (`lib/`)

This directory contains **platform-agnostic, dependency-disciplined libraries** intended for broad reuse across Bharat-OS.

## The Golden Rule

**Code in this directory MUST NOT depend on kernel internals, physical memory allocation, trap handling, or privileged CPU state.**

## Authorized Consumers
* User-space runtimes
* System services
* Host tools and test frameworks
* Kernel (only under strict usage guidelines)

## Requirements for Code Here
1. **Zero Kernel State:** You cannot include or use kernel globals or locks.
2. **No Implicit Privileges:** You cannot assume execution in ring 0 or with interrupts disabled.
3. **Explicit Allocations:** You must not call `kalloc` or other kernel-internal allocators directly. If your library needs memory, the caller must pass it in or provide an allocator function pointer.
4. **Host Testable:** Everything here must be buildable and testable on the host via `cmake --preset host-test`.

If your code needs kernel privileges, physical memory, or scheduler state, it belongs in `kernel/src/lib/`.
