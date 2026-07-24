---
title: Runtime ISA Extension Leverage in Core Kernel: Gap Analysis (x86_64, arm64, riscv64)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
  - gap_analysis
see_also:
  - README.md
---
# Runtime ISA Extension Leverage in Core Kernel: Gap Analysis (x86_64, arm64, riscv64)

**Date:** 2026-03-19  
**Scope:** Whether Bharat-OS should leverage additional ISA extensions discovered at runtime for architecture-specific kernel fast paths.

---

## 1) Problem Statement

You asked whether the kernel can detect "extra" ISA features at runtime and use them only on hardware that supports them (otherwise safely fall back), so the system can run faster.

**Short answer:** Yes, this is the right strategy for all three major architectures (x86_64, arm64, riscv64), and it can be done safely with per-feature gating + fallback implementations.

---

## 2) Current Gap in Bharat-OS

The current CPU capability interface is present but minimal:

- `arch_cpu_caps_init()` exists as an API surface in `core/kernel/include/core/arch/arch_cpu_caps.h`.
- Per-architecture implementations are currently stubs in:
  - `core/arch/x86/x86_64/cpu_caps.c`
  - `core/arch/arm/arm64/cpu_caps.c`
  - `core/arch/riscv/riscv64/cpu_caps.c`

This means the kernel has **no reliable runtime feature map** to decide whether to enable optimized code paths.

---

## 3) Architecture-by-Architecture Analysis

## 3.1 x86_64

### Candidate runtime extensions to leverage
- Memory ops: SSE2 baseline (already common), AVX2, AVX-512 (careful due to context switch cost).
- Crypto: AES-NI, PCLMULQDQ, SHA extensions.
- Synchronization/perf: FSGSBASE, INVPCID, PCID, RDPID, CLWB/CLFLUSHOPT.

### Kernel opportunities
- Faster `memcpy`/`memset` in page allocator, IPC copy paths, and zero-page setup.
- Faster kernel crypto helper primitives for integrity/authentication paths.
- Better TLB/cache maintenance routines when PCID/INVPCID available.

### Risks
- AVX/AVX-512 state management overhead can hurt if enabled blindly.
- Need strict fallback to scalar implementations when unsupported.

### Recommendation
- Start with **low-risk features** (AES-NI, PCLMUL, SHA, PCID/INVPCID).
- Keep AVX-class acceleration opt-in behind measured thresholds.

---

## 3.2 arm64

### Candidate runtime extensions to leverage
- SIMD/crypto: NEON/ASIMD, AES, SHA1/SHA2/SHA3, PMULL.
- Memory/atomic: LSE atomics.
- Compute/vector: SVE/SVE2 (careful with context handling).
- Pointer/security: PAC/BTI/MTE where platform policy allows.

### Kernel opportunities
- LSE-based lock/atomic paths for better multicore scalability.
- Crypto acceleration using ARMv8 crypto instructions.
- Optional SVE/SVE2 routines for large block operations (with cost controls).

### Risks
- SVE context size may significantly increase context switch overhead.
- Some features can be EL/firmware-policy dependent.

### Recommendation
- Prioritize **LSE + crypto extensions** first.
- Defer aggressive SVE kernel use until lazy/optimized save-restore is mature.

---

## 3.3 riscv64

### Candidate runtime extensions to leverage
- Core integer/perf/security: Zba, Zbb, Zbc, Zbs, Zicbom, Zicboz.
- Crypto: Zk* extensions (e.g., AES/SHA families where available).
- Vector: V (RVV), with variable VLEN.

### Kernel opportunities
- Bitmanip for scheduler/bitmap/capability operations.
- Cache-block management ops for DMA/cache control optimization.
- Vector-accelerated bulk copy/clear and checksum routines.

### Risks
- Fragmentation of extension availability across vendors/platforms.
- RVV has variable vector length: routines must be VLEN-agnostic and tested.

### Recommendation
- Build a **strict extension bitmap model** from ISA strings/CSR probing.
- Enable only a minimal stable subset first (bitmanip + selected cache ops).

---

## 4) Implementation Model (Common Across All 3 Architectures)

1. **Detect:** Populate a per-CPU and per-system feature bitmap during early boot.
2. **Validate:** Check dependencies (e.g., instruction + state management + policy).
3. **Dispatch:** Route hot routines through function pointer tables/static keys.
4. **Fallback:** Always keep baseline generic C/asm path.
5. **Expose:** Provide read-only capability view to subsystems for tuning decisions.

---

## 5) Suggested Code Changes in This Repo

### Phase 1 (Foundation)
- Expand `core/kernel/include/core/arch/arch_cpu_caps.h` with:
  - enumerated feature IDs,
  - query helpers (`arch_cpu_has(feature)`),
  - optional per-cpu feature record.
- Replace stubs in:
  - `core/arch/x86/x86_64/cpu_caps.c`
  - `core/arch/arm/arm64/cpu_caps.c`
  - `core/arch/riscv/riscv64/cpu_caps.c`
  with real probe logic.

### Phase 2 (Low-risk acceleration)
- Introduce a small `core/kernel/src/core/arch/common/fastops_dispatch.c` (or similar) for dispatch hooks.
- Wire architecture-specific fast paths for:
  - `memcpy`/`memset`/page-clear,
  - selected crypto primitives,
  - selected atomic/lock internals.

### Phase 3 (Advanced)
- Add controlled vector path usage (AVX/SVE/RVV) with measured thresholds.
- Add runtime telemetry counters to verify real-world benefit and regressions.

---

## 6) Decision Rule for Core Kernel Usage

Use this policy:

- **Can we leverage runtime-only ISA extensions in core kernel code?** → **Yes**.
- **Condition:** only through capability-gated dispatch with guaranteed safe fallback.
- **Never:** emit unsupported instructions in unconditional generic paths.

In practical terms:

- Baseline kernel always boots on minimum ISA for target architecture.
- Optimized routines activate only when runtime probe confirms support.
- If uncertain (virtualized/quirky firmware), stay on conservative path.

---

## 7) Prioritized Backlog for Bharat-OS

1. Implement non-stub feature probing in all 3 arch `cpu_caps.c` files.
2. Add a single canonical capability query API in `arch_cpu_caps.h`.
3. Land one accelerated path per architecture (crypto or memory) with A/B benchmark hooks.
4. Add CI/QEMU coverage for "feature present" and "feature absent" boot modes.
5. Document per-feature fallbacks and context-switch implications.

---

## 8) Final Recommendation

For Bharat-OS, runtime ISA feature leverage is a **high-value optimization direction** and should be adopted as a kernel design pattern. It is especially useful because your current code structure already contains architecture split points where capability probing and dispatch can be inserted cleanly.

The key is to treat runtime ISA usage as an **optimization layer**, not as a correctness dependency.
