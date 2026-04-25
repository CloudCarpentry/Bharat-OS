---
title: Compiler Safety and Optimization
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Compiler Safety and Optimization

In kernel code, the optimization order must be:

1. **Correct under freestanding compilation**
2. **Stable across compiler versions**
3. **Efficient after correctness is locked**
4. **Architecture-specialized only in isolated files**

This matches Bharat-OS's core direction: minimal kernel, clear boundaries, arch-specific implementation in `core/arch/`, abstraction contracts in `core/hal/`, and platform wiring in `core/platform/`.

## The Challenge

Newer Clang versions may apply more aggressive loop recognition, builtin lowering, and target-specific optimization than older toolchains. Bharat-OS must therefore treat core memory/string helpers and early-boot parsers as compiler-sensitive code, and harden them using freestanding mode, targeted builtin suppression, scalar-safe fallbacks, and architecture-isolated optimized paths.

With Clang, `-ffreestanding` changes compiler behavior significantly (setting `__STDC_HOSTED__` to `0` and disabling most builtins), but still allows Clang to generate calls to `memcpy`, `memmove`, and `memset`. Therefore, these **must exist** in the runtime/kernel support layer.

For Bharat-OS, the safe interpretation is:
* `-ffreestanding` is **necessary**
* `-fno-builtin-*` is still worth keeping for **defense in depth**, especially for the exact functions being implemented or wrapped
* Never rely only on toolchain version checks

## Bharat-OS Strategy

### 1. Split memory helpers into tiers

Use 3 tiers:

**Tier A: canonical scalar-safe functions**
* always present
* boring, loop-based, hardened
* used during early boot and as fallback

**Tier B: arch-optimized functions**
* in `core/arch/...`
* only compiled when that target is active
* NEON/RVV/SSE/REP MOVSB or similar later

**Tier C: dispatch wrapper**
* picks safe scalar early
* picks optimized path only after CPU/features/runtime are known

### 2. Do not depend on compiler version numbers

Instead of checking compiler version numbers (e.g. `#if CLANG_VERSION >= 22`), use feature-detection macros like `__has_builtin`. This is much safer across Windows Clang, WSL Clang, future LLVM upgrades, and non-Clang fallback compilers.

### 3. Use function-level hardening

For the tiny set of critical functions, harden them individually using attributes like `noinline` and `used`.

### 4. Keep public names thin

Do not put all logic directly into `memset`, `memcpy`, `memmove`. Use wrappers that call the scalar implementation or dispatch to optimized versions. This reduces the chance of the compiler "helping" inside the public libc-like symbol itself.

### 5. Add targeted compile flags

* **kernel-wide**: `-ffreestanding`
* **critical helper files**: extra `-fno-builtin-*`
* **general code**: allow normal optimization

### 6. Separate "early boot safe" from "normal runtime fast"

Early boot must use only scalar-safe routines with no vector assumptions or platform runtime dependencies. Post-init allows for arch-specialized routines (e.g., ARM64 DC ZVA, x86_64 REP MOVSB).

### 7. Add explicit architecture gating everywhere

Never let common code accidentally compile target-specific inline asm. `core/arch/` owns ISA-specific implementations.

### 8. Put the policy in one header

Use `core/kernel/include/bharat/compiler_safety.h` to provide one consistent contract across all builds.

### 9. Add per-file optimization policy

For the most sensitive files, use a safer optimization profile (e.g. `-O1` or `-O2`, but never `-O0` unless debugging).

### 10. Add CI checks

Fail CI if `memset` contains a direct call to `memset`, or `memcpy` to `memcpy`.

## Bharat-OS Coding Standard

1. Any implementation of `memset`, `memcpy`, `memmove`, `strlen`, `strcmp`, early string parsing, and boot memory clear/copy must have a **generic scalar-safe implementation**.
2. These functions must use `BHARAT_NOINLINE`, hardened pointer access, and targeted `-fno-builtin-*` on their compilation unit.
3. Optimized arch versions are optional and must never be the only implementation.
4. Common code must use feature checks, not compiler version checks.
5. No target-specific inline assembly in shared files without full target guards.
6. Early boot must default to generic safe routines until core/platform/CPU state is confirmed.

**Efficiency without breaking safety:** Safe scalar first, optimized later by dispatch. This provides reliable boot on mismatched toolchains and fast steady-state runtime.
