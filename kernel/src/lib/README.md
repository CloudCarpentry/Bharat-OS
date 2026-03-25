# Kernel-Private Libraries (`kernel/src/lib/`)

This directory contains **kernel-private utilities and helpers** explicitly bound to the kernel implementation.

## The Golden Rule

**Code in this directory is NOT a shared API surface and MUST NOT be used by user-space runtimes, system services, or host tools.**

## Authorized Consumers
* Core kernel subsystems
* Architecture and HAL bridging logic within the kernel
* In-kernel boot and test frameworks

## Characteristics of Code Here
1. **Privileged Execution:** It may freely use kernel internals, spinlocks, scheduler state, IRQ masks, and CPU-local contexts.
2. **Implicit Allocations:** It may directly depend on kernel allocators (`kalloc`, `vm_alloc`).
3. **Fault Handling:** It may contain fault-safe formatting or panic helpers specific to the kernel domain.
4. **Boot Dependencies:** It may rely on boot-stage assumptions if properly documented.

If your code is platform-agnostic, allocation-free (or requires explicit allocator injection), and needs to be shared with user-space or host tools, it belongs in the top-level `lib/` directory.
