---
title: "Kernel Coding Standards: Memory and String Primitives"
status: "active"
owner: "Bharat-OS Team"
reviewers: ["Core Architecture Team"]
version: "1.0"
last_updated: "2024-03-23"
tags: ["standards", "kernel", "libc", "memory"]
---

# Kernel Coding Standards: Memory and String Primitives

## Overview
This architectural contract outlines the strict distinction and correct usage of memory and string operations within the Bharat-OS kernel. Due to the freestanding nature of the kernel and the necessity for distinct hardware-accelerated versus safe-scalar operations across diverse execution profiles, standard libc `<string.h>` is explicitly forbidden.

## The Dual-Layer Approach

Bharat-OS employs a dual-layer strategy for memory primitives:

### 1. The Generic Kernel Standard (`<lib/string.h>`)
**Purpose:** Provides familiar POSIX-like string and memory operations (`memset`, `memcpy`, `memmove`, `memcmp`, `strlen`, etc.) for generic kernel code.
**When to Use:**
- File systems (e.g., `ramfs.c`)
- High-level memory management (e.g., `slab.c`)
- Debugging and diagnostics (e.g., `panic.c`)
- General kernel subsystem logic that executes in standard context.
**Under the Hood:** These functions default to calling architecture-specific hardware-accelerated operations (`arch_memset`, `arch_memcpy`) with safe default flags (`ARCH_MEMOP_F_DEFAULT | ARCH_MEMOP_F_NO_SIMD`).

### 2. The Architectural Hardware Primitives (`<arch/memops.h>`)
**Purpose:** Exposes raw architecture-specific implementations (accelerated or scalar) that can be precisely controlled via execution context flags (`ARCH_MEMOP_F_EARLY_BOOT`, `ARCH_MEMOP_F_IRQ_SAFE`, etc.).
**When to Use:**
- Early boot code (before hardware accelerators are initialized)
- Architecture-specific setup or trap handling
- Page table zeroing, TLB invalidation, and core MMU routines where SIMD or DMA accelerators MUST NOT be used.
- HAL implementations.
**Available Primitives:**
- `arch_memset`, `arch_memcpy`, `arch_memmove`: Flag-driven dispatched operations.
- `arch_memset_scalar`, `arch_memcpy_scalar`, `arch_memmove_scalar`: Pure-software, definitively safe, unaccelerated implementations.

## Rules
1. **Never include `<string.h>`** inside the kernel.
2. Generic kernel code must include `"lib/string.h"` and use standard functions (e.g., `memset`).
3. Hardware-sensitive code (early boot, TLB, critical IRQ paths) must include `"arch/memops.h"` and explicitly use `arch_memset_scalar` or `arch_memset` with the appropriate safe flags.
