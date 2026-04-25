# Runtime ISA Extension Strategy → Production Tasklist

**Date:** 2026-04-25  
**Reviewed doc:** `docs/reviews/gap_analysis/runtime_isa_extension_strategy.md`  
**Focus requested:** low-level ISA implementation work in `core/arch/*` and HAL abstraction in `core/hal/*`.

---

## 1) What changed since the original gap analysis

The referenced strategy doc assumes CPU capability probes are still stubs. In the current tree, runtime CPU-capability plumbing exists and is partially implemented:

- Shared capability model and query API are present in `core/kernel/include/arch/arch_cpu_caps.h`.
- Per-arch probe implementations exist in:
  - `core/arch/x86/x86_64/cpu_caps.c`
  - `core/arch/arm/arm64/cpu_caps.c`
  - `core/arch/riscv/riscv64/cpu_caps.c`
- HAL-level mapping/publication is already wired in:
  - `core/hal/common/cpu_features.c`
  - `core/hal/common/discovery.c`

So the production gap is no longer “build API from zero”; it is now **hardening, correctness gating, heterogeneous CPU handling, and accelerated path dispatch integration**.

---

## 2) Production-grade implementation tasks

## 2.1 Cross-architecture tasks (must do first)

### A. Capability source-of-truth and dependency validation

**Goal:** make `raw` vs `usable` semantics strict and auditable.

**Code work**
- Add explicit dependency helpers in arch layer (e.g., feature implies feature) and enforce before setting `usable` bits.
- Add policy hook(s) so security/perf policy can disable usable bits without changing raw probes.

**Primary files**
- `core/kernel/include/arch/arch_cpu_caps.h`
- `core/arch/common/cpu_caps_state.c`
- `core/hal/common/cpu_features.c`

### B. Per-CPU heterogeneity correctness

**Goal:** prevent unsupported instructions on mixed-feature systems.

**Code work**
- Ensure all optimized kernel paths check either:
  - `system_all` (safe globally), or
  - current-CPU bit when execution is strictly CPU-local and non-migrating.
- Add API helper names in arch/HAL to make scope explicit (`_system_all`, `_system_any`, `_current_cpu`).

**Primary files**
- `core/kernel/include/arch/arch_cpu_caps.h`
- `core/hal/include/hal/hal_cpu_features.h`
- callsites in memops/crypto/atomic dispatch modules.

### C. Common fast-path dispatch table

**Goal:** centralized dispatch so all hot paths use one gating strategy.

**Code work**
- Introduce `core/arch/common/fastops_dispatch.c` + header.
- Dispatch targets:
  - `memcpy`/`memset`/page-clear
  - crypto primitive selector
  - atomics/lock primitive selector
- Tie dispatch decision to `arch_cpu_caps_system_all()` by default.

**Primary files**
- `core/arch/common/fastops_dispatch.c` (new)
- `core/arch/common/memops_scalar.c`
- `core/kernel/include/arch/memops.h`
- `core/lib/runtime/crypto/crypto_dispatch.c`

### D. Telemetry and verification hooks

**Goal:** prove acceleration is a net win and safe.

**Code work**
- Per-path counters (selected/fallback/hit-rate) exported via HAL discovery or debug endpoint.
- A/B boot knobs to force-disable each acceleration family.

**Primary files**
- `core/hal/common/discovery.c`
- arch-specific boot/options parsing paths
- runtime debug/service exposure module (existing telemetry surface).

---

## 2.2 x86_64 tasks (`core/arch/x86/x86_64` + `core/hal/x86_64`)

### A. Correct CPUID leaf usage and gating fixes

**Code work**
- Fix PCID bit detection location (currently read from CPUID leaf 7 ECX; should be leaf 1 ECX).
- Add/verify XSAVE+XCR0 gating for every vector-level path actually emitted.
- Split “detected” vs “enabled by kernel context policy” for AVX/AVX2 (and future AVX-512).

**Primary files**
- `core/arch/x86/x86_64/cpu_caps.c`
- `core/arch/x86/x86_64/ext_state.c`
- `core/arch/x86/x86_64/context_switch.S`

### B. TLB/context acceleration integration

**Code work**
- Wire `PCID`/`INVPCID` to set `ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX` when full prerequisites hold.
- Use this in x86 HAL page-table/TLB shootdown paths.

**Primary files**
- `core/arch/x86/x86_64/cpu_caps.c`
- `core/hal/x86_64/hal_pt_x86_64.c`
- `core/hal/x86_64/apic.c` (if shootdown policy ties here)

### C. First production fast paths

**Code work**
- Keep `rep movsb/stosb` baseline fast-string path.
- Add AES/PCLMUL-backed crypto helper integration where runtime-gated.

**Primary files**
- `core/arch/x86/x86_64/memops_fast_string.c`
- `core/hal/x86_64/hash/hash_x86_64.c`
- `core/lib/runtime/crypto/crypto_dispatch.c`

---

## 2.3 arm64 tasks (`core/arch/arm/arm64` + `core/hal/arm64`)

### A. Feature decoding completeness

**Code work**
- Expand decode coverage for SHA2/SHA3 variants and expose explicit bits.
- Add strict EL/firmware/policy checks before promoting raw→usable for sensitive features.

**Primary files**
- `core/arch/arm/arm64/cpu_caps.c`
- `core/kernel/include/arch/arch_cpu_caps.h` and arm64 feature defs

### B. LSE and atomic fast-path routing

**Code work**
- Route lock/atomic internals through LSE-optimized path when `ARCH_CPU_FEAT_ARM64_LSE` usable.
- Keep LL/SC fallback mandatory.

**Primary files**
- `core/arch/arm/arm64/cpu_caps.c`
- arm64 lock/atomic implementation files (where atomics are defined)
- `core/hal/common/cpu_features.c`

### C. SVE/SVE2 production-readiness track

**Code work**
- Keep SVE/SVE2 as raw-only until full context-switch support is verified.
- When enabling usable bits, ensure save/restore logic and preemption model are complete.

**Primary files**
- `core/arch/arm/arm64/ext_state.c`
- `core/arch/arm/arm64/context_switch.S`
- `core/arch/arm/arm64/cpu_caps.c`

---

## 2.4 riscv64 tasks (`core/arch/riscv/riscv64` + `core/hal/riscv64`)

### A. Replace compile-time-only extension model

**Code work**
- Move from compile-time `BHARAT_ISA_FEATURE_*` flags to runtime extension discovery via DT/SBI/HWCAP source.
- Preserve compile-time flags only as fallback/developer override.

**Primary files**
- `core/arch/riscv/riscv64/cpu_caps.c`
- `core/hal/riscv64/discovery.c`
- `core/hal/riscv64/boot_sbi.c`
- `core/hal/riscv64/topology_fdt.c`

### B. Minimal stable extension set enablement

**Code work**
- Production-enable Zba/Zbb/Zbc/Zbs feature use in bit operations.
- Add zicbo* handling in DMA/cache maintenance where platform says safe.

**Primary files**
- `core/arch/riscv/riscv64/cpu_caps.c`
- `core/hal/riscv64/dma.c`
- bitmap/scheduler/capability utility modules that can exploit bitmanip.

### C. RVV readiness gates

**Code work**
- Keep RVV raw-only until vector context and VLEN-agnostic routines are validated.
- Add per-platform opt-in knob before setting usable RVV bits.

**Primary files**
- `core/arch/riscv/riscv64/ext_state.c`
- `core/arch/riscv/riscv64/context_switch.S`
- `core/arch/riscv/riscv64/cpu_caps.c`

---

## 2.5 HAL abstraction tasks (`core/hal`)

### A. Unify ISA capability surface for subsystems

**Code work**
- Decide one canonical HAL API (`hal_cpu_features` vs `hal_isa_caps`) and remove duplicate/parallel reporting.
- Keep raw/usable + system_all/system_any semantics visible to subsystems.

**Primary files**
- `core/hal/include/hal/hal_cpu_features.h`
- `core/hal/include/hal/hal_isa_caps.h`
- `core/hal/common/cpu_features.c`
- `core/hal/common/discovery.c`

### B. HAL-level dispatch helpers for safe usage

**Code work**
- Add HAL helper wrappers for common checks used by runtime code paths:
  - `hal_accel_can_use_vector_global()`
  - `hal_accel_can_use_crypto_global()`
  - `hal_accel_has_bitmanip_any_cpu()` etc.

**Primary files**
- `core/hal/include/hal/*.h`
- `core/hal/common/*.c`

### C. Test/CI matrix integration

**Code work**
- Add QEMU/CI modes that explicitly force capability on/off combinations.
- Verify dispatch and fallback behavior per mode.

**Primary files**
- build/test scripts in repo root
- architecture-specific run/test scripts and HAL discovery assertions.

---

## 3) Recommended execution order (production roadmap)

1. **Correctness hardening** (dependency checks, heterogeneity-safe APIs, CPUID/ID register fixes).  
2. **Dispatch layer** (single fastops dispatch + mandatory fallbacks).  
3. **Low-risk acceleration** (x86 AES/PCLMUL, arm64 LSE+crypto, riscv bitmanip).  
4. **CI coverage** (feature present/absent permutations).  
5. **Advanced vectors** (AVX-class/SVE/RVV only after context-state readiness).  

---

## 4) Definition of done for “production-grade ISA leverage”

A feature is production-ready only when all are true:

- Runtime detection is accurate on real hardware/VM and represented in per-CPU + system aggregates.
- `usable` bit is gated by OS context-management + security/perf policy.
- Hot path uses centralized dispatch with guaranteed scalar fallback.
- CI verifies both accelerated and fallback execution paths.
- Telemetry proves no regression under mixed/unsupported systems.
