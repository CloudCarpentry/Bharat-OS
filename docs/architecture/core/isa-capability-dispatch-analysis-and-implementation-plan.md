# ISA Capability & Dispatch Analysis + Implementation Plan

## Purpose

This document captures a code+document analysis of Bharat-OS ISA feature handling and provides a concrete implementation backlog.

Guiding model:

> detect once, normalize centrally, dispatch cleanly, always keep a safe baseline path.

---

## Analysis inputs

### Code reviewed

- `kernel/include/arch/arch_cpu_caps.h`
- `arch/common/cpu_caps_state.c`
- `arch/x86/x86_64/cpu_caps.c`
- `arch/arm/arm64/cpu_caps.c`
- `arch/riscv/riscv64/cpu_caps.c`
- `arch/arm/arm32/cpu_caps.c`
- `kernel/include/hal/hal_discovery.h`
- `hal/common/discovery.c`
- `kernel/include/arch/memops.h`
- `arch/common/memops_scalar.c`
- `arch/x86/x86_64/memops.c`
- `arch/arm/arm64/memops.c`
- `arch/riscv/riscv64/memops.c`
- `hal/include/hal/hal_isa_caps.h`
- `hal/include/hal/hal_cpu.h`

### Docs reviewed

- `docs/architecture/folder_structure.md`
- `docs/architecture/core/hardware-capability-model.md`
- `docs/architecture/shared/lib_roadmap.md`

---

## Current-state matrix (implemented vs not implemented)

## 1) Capability model and APIs

### Implemented

- Generic CPU capability bitset model exists (`arch_cpu_caps_t`, `arch_cpu_caps_record_t`) with common semantic features plus arch-private windows.
- Query helpers exist for boot/current/system scope (`arch_cpu_caps_boot/current/system_all/system_any`, `arch_cpu_has`).
- HAL discovery publishes normalized accelerator masks (`raw_any/all`, `usable_any/all`) through `system_discovery_t.accel`.

### Not implemented / partial

- No canonical HAL contract for CPU features at `hal/include/hal/hal_cpu_features.h` (or equivalent strongly typed API).
- Existing `hal_isa_caps.h` is minimal (`isa_name`, `has_mmu`, `has_mpu`, `min_alignment`) and does not represent modern feature classes.
- No per-cluster/package feature record in HAL API.
- No user/service-facing stable query API for capabilities (beyond internal discovery structs).

### Needs improvement

- Normalize around generalized HAL feature classes (vector, scalable vector, matrix, memory tagging, branch protection, crypto, bitmanip, cache block ops) and map ISA details in `arch/`.

---

## 2) Detection pipeline

### Implemented

- x86_64 detection reads CPUID + XCR0 and correctly gates AVX/AVX2/FMA usability on OS-enabled XSAVE state.
- arm64 detection reads ID registers and distinguishes raw vs usable for SVE/SVE2.
- riscv64 has initial capability population and optional extension bits through build defines.

### Not implemented / partial

- arm32 detector is stubbed (`arch_cpu_caps_init(void) {}`), so no real capability discovery.
- AP (secondary CPU) detection is not implemented (`arch_cpu_caps_init_ap()` empty in multiple arch files).
- System aggregation is placeholder: `arch_cpu_caps_system_finalize()` currently mirrors boot CPU instead of true all/any reduction across online CPUs.
- RISC-V runtime detection is incomplete; comments indicate expected DT/SBI probing but current code largely compile-time gated for Zb* bits.

### Needs improvement

- Detect per CPU at boot (BSP + APs), persist per-core records, compute system-wide `all` and `any` masks from real online CPUs.
- Add boot-time capability dump (human readable + machine parseable).

---

## 3) Dispatch model

### Implemented

- Mandatory safe scalar memory path exists (`arch_memcpy_scalar`, `arch_memset_scalar`, `arch_memmove_scalar`).
- Per-arch memops dispatch exists (`arch_memcpy/memset/memmove`) with conservative early-boot/IRQ-safe fallback behavior.
- x86_64 uses fast-string path for memcpy/memset in non-sensitive contexts.
- riscv64 uses GPR bulk copies outside early-boot/IRQ-safe contexts.

### Not implemented / partial

- No centralized ops-table install step (e.g., `memops_ops`, `crypto_ops`, `bitops_ops`, `cacheops_ops`) selected once at boot.
- arm64 currently forces scalar memops due to alignment concerns; optimized path exists but is not enabled in dispatcher.
- memops dispatch does not currently key off unified capability profile from HAL (mostly flag/context based).

### Needs improvement

- Replace ad-hoc path selection with boot-selected ops table per subsystem (memops first, then bitops/crypto/cacheops).

---

## 4) Architectural boundary compliance (`arch/`, `hal/`, `platform/`)

### Implemented

- Most ISA probing logic is in `arch/*/cpu_caps.c`, aligned with boundary goals.
- HAL common layer (`hal/common/discovery.c`) translates arch features into higher-level masks.

### Not implemented / partial

- Repository still has arch-specific directories under `hal/` (known and documented boundary blur).
- Capability normalization currently lives partly in kernel headers (`kernel/include/hal/hal_discovery.h`) rather than a tighter HAL contract package.

### Needs improvement

- Keep raw ISA parse and feature probing in `arch/`.
- Move/introduce a canonical HAL-facing feature contract in `hal/include/hal/` and make all callers consume that API.

---

## Gap summary (priority ordered)

1. **P0:** Complete multi-core detection correctness (AP probe + real system aggregate).
2. **P0:** Define canonical HAL CPU feature contract and API.
3. **P1:** Introduce boot-time ops-table dispatch installation for memops/bitops/crypto/cacheops.
4. **P1:** Replace RISC-V compile-time extension assumptions with runtime/firmware probing where possible.
5. **P2:** Expand hardening and advanced features (MTE/PAC/BTI, CET-like, scalable vectors, matrix).
6. **P2:** Add profile/policy gates for expensive or compatibility-impacting features.

---

## Implementation plan and task backlog

## Phase 1 — Correctness + core capability contract

### Task 1.1: Add canonical HAL CPU feature API

**Add files**

- `hal/include/hal/hal_cpu_features.h`
- `hal/common/cpu_features.c`

**Deliverables**

- Feature enum with architecture-neutral classes.
- Per-core + system feature-set query APIs.
- Helpers like `hal_cpu_has_feature(cpu_id, feat)` and `hal_cpu_has_system_feature(feat, scope_any_or_all)`.

**Acceptance criteria**

- No direct ISA macro checks outside `arch/` and arch mapping units for new call sites.
- Build passes for x86_64, arm64, riscv64 targets.

### Task 1.2: Real AP probing and aggregation

**Change files**

- `arch/*/*/cpu_caps.c` (all supported ISAs)
- `arch/common/cpu_caps_state.c`

**Deliverables**

- Implement `arch_cpu_caps_init_ap()` for supported ISAs.
- `arch_cpu_caps_system_finalize()` performs real AND/OR reduction over online CPUs.

**Acceptance criteria**

- On heterogenous feature simulation tests, `system_all` and `system_any` diverge correctly.
- Boot logs show per-core and aggregate masks.

### Task 1.3: Arm32 baseline detector

**Change file**

- `arch/arm/arm32/cpu_caps.c`

**Deliverables**

- Minimal ARM32 baseline capability discovery (atomics/barriers/vector availability as applicable).

**Acceptance criteria**

- Non-empty, deterministic arm32 capability record; no stub implementation remains.

---

## Phase 2 — Dispatch architecture

### Task 2.1: Memops ops-table install

**Add files**

- `hal/common/memops/ops.c`
- `hal/include/hal/hal_memops_ops.h`

**Deliverables**

- Boot-time selection of `memcpy/memset/memmove/page_clear/page_copy` function table.
- Safe fallback table always available.

**Acceptance criteria**

- Hot callsites use installed ops table, not scattered feature branches.
- Early-boot/IRQ-safe paths remain deterministic and scalar-safe.

### Task 2.2: Bitops/Crypto/Cacheops ops tables

**Add files**

- `hal/common/bitops/ops.c`
- `hal/common/crypto/ops.c`
- `hal/common/cacheops/ops.c`

**Deliverables**

- Same dispatch pattern as memops.
- Arch implementations remain in `arch/`; HAL only selects and exposes contracts.

**Acceptance criteria**

- No direct ISA feature conditionals in subsystem logic outside dispatch/install units.

---

## Phase 3 — Policy and profile integration

### Task 3.1: Profile-gated hardening and heavy extensions

**Deliverables**

- Profile flags controlling security hardening features and heavy compute features.
- Runtime policy docs mapping profile class to feature allowlist.

**Acceptance criteria**

- Secure profile can enable hardening (where implemented) while minimal profile remains compatible.

### Task 3.2: User/service capability query contract

**Deliverables**

- Stable query ABI for service/runtime selection decisions.

**Acceptance criteria**

- At least one service and one runtime component consume the new query API for dispatch decisions.

---

## Tracking board template (ticket-ready)

- [ ] BO-ISA-001 Canonical HAL CPU feature contract (`hal_cpu_features.h`)
- [ ] BO-ISA-002 AP capability probing on x86_64/arm64/riscv64
- [ ] BO-ISA-003 True `system_all`/`system_any` aggregation
- [ ] BO-ISA-004 arm32 capability detector baseline
- [ ] BO-ISA-005 Memops ops-table dispatch install
- [ ] BO-ISA-006 Bitops ops-table dispatch install
- [ ] BO-ISA-007 Crypto ops-table dispatch install
- [ ] BO-ISA-008 Cacheops ops-table dispatch install
- [ ] BO-ISA-009 Boot capability dump (per-core + aggregate)
- [ ] BO-ISA-010 Profile-gated hardening/heavy-feature policy
- [ ] BO-ISA-011 User/service capability query API

---

## Recommended implementation order

1. BO-ISA-001/002/003 first (foundational correctness).
2. BO-ISA-005 next (highest practical performance impact with low policy complexity).
3. BO-ISA-006/007/008 after memops pattern is validated.
4. BO-ISA-004 in parallel if arm32 target is active.
5. BO-ISA-009/010/011 to operationalize and expose capability-driven behavior.

---

## Definition of done (for this initiative)

- Every supported architecture has:
  - a mandatory safe baseline path,
  - boot-time feature detection,
  - normalized HAL feature advertisement,
  - boot-selected dispatch tables for hot primitives,
  - and verified generic fallback behavior.
- No ISA-fragmented conditionals are scattered through kernel/services fast paths.
- Feature enablement policy is profile-driven and auditable.

---

## Implementation update (2026-04-22)

Completed in this change set:

- Added canonical HAL CPU feature query contract:
  - `hal/include/hal/hal_cpu_features.h`
  - `hal/common/cpu_features.c`
- Completed AP capability probing hooks for:
  - `arch/x86/x86_64/cpu_caps.c`
  - `arch/arm/arm64/cpu_caps.c`
  - `arch/riscv/riscv64/cpu_caps.c`
  - `arch/arm/arm32/cpu_caps.c`
- Implemented true `system_all` / `system_any` aggregation over online CPUs in:
  - `arch/common/cpu_caps_state.c`
- Added host verification test:
  - `tests/host/test_hal_cpu_features.c`

Remaining high-priority follow-up:

- Boot-time capability dump (`BO-ISA-009`)
- Ops-table dispatch installation for memops/bitops/crypto/cacheops (`BO-ISA-005`..`008`)
- Runtime RISC-V firmware-backed probing beyond compile-time extension gates
