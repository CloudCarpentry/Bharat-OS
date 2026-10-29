# Bharat-OS Architecture & Code Review Index

This directory maintains the historical, completed, and active architectural reviews, audits, and gap analyses for Bharat-OS.

> **Ground Truth Rule (AGENTS.md):** Historical reviews from early 2026 are preserved for provenance, but are **not authoritative** for current code state. The active convergence baseline is commit `3041933a` (October 14, 2026). Prior reviews detailing missing PMM caches, absent TLB shootdown protocols, unvalidated syscall tables, or basic mock services are **superseded and archived**.

---

## 1. Completed & Hardened Reviews

The following reviews represent identified defects or architectural milestones that have been **fully resolved, implemented, and verified** in the codebase:

| Review Document | Initial Scope | Status & Resolution Evidence |
|---|---|---|
| [`code_inconsistencies.md`](code_inconsistencies.md) | Header include placement, empty comments, naming convention violations | **COMPLETED / RESOLVED** (2026-10-14). Cleaned 100% of empty comments (0 remaining), converted `fb_input_showcase.c` to `snake_case`, and moved mid-file `#include` directives across kernel/arch/services/drivers. Enforced by [`tools/lint/check_code_hygiene.py`](../../tools/lint/check_code_hygiene.py). |
| [`tlb-shootdown-hardening-review.md`](tlb-shootdown-hardening-review.md) | Unbounded TLB broadcast waits and cross-core lock holding | **COMPLETED / HARDENED**. Separated transport from orchestration. Implemented monotonic HAL deadlines (`bh_ktime_now()`), core-local pending tracking, and bounded timeouts. |
| [`pmm-per-core-hardening-review.md`](pmm-per-core-hardening-review.md) | Global PMM lock contention and fixed core limits | **COMPLETED / HARDENED**. Scaled to `BHARAT_MAX_CPUS` (32 cores). Implemented per-core `pcache` magazines (high=128, low=64, batch=32) and MPSC lock-free remote-free inboxes. |
| [`syscall-production-hardening-review.md`](syscall-production-hardening-review.md) | Scaffolded syscall dispatch, direct user pointer dereferences | **COMPLETED / HARDENED**. Canonical authority unified in `interface/contracts/abi/native_syscalls.json` with SHA-256 ABI lock. Enforced fault-safe `bh_usercopy_from_user` and centralized capability validation. |
| [`syscall_conflict_analysis_2026-04-26.md`](syscall_conflict_analysis_2026-04-26.md) | Duplicate syscall headers and cross-architecture symbol conflicts | **COMPLETED / ARCHIVED**. Unified on single UAPI ABI authority. Fixed duplicate symbols in x86/ARM HAL. All 5 canonical QEMU targets boot cleanly. |
| [`core_arch_production_grade_review_2026-04-25.md`](core_arch_production_grade_review_2026-04-25.md) | Hardcoded FDT fallbacks, `#ifdef` leakage in HAL, Multiboot validation | **COMPLETED / ARCHIVED**. All 6 defects (01–06) fixed. Verified 5-target QEMU boot passing. |
| [`SMP-P1-001-URPC-Memory-Ordering-Audit.md`](SMP-P1-001-URPC-Memory-Ordering-Audit.md) | Loose `volatile` compiler barriers causing race conditions on ARM64/RV64 | **COMPLETED / HARDENED**. Implemented C11 `<stdatomic.h>` with explicit acquire-release semantics for all single-producer/single-consumer (SPSC) channels. |
| [`boot-flow-p0-001-recovery.md`](boot-flow-p0-001-recovery.md) | Misleading init success output without actual process creation | **COMPLETED / HARDENED**. Restored genuine ELF loading, capability bootstrap handoff, and userspace process creation via `process_manager`. |
| [`boot/BOOT-P0-001-runtime-truth.md`](boot/BOOT-P0-001-runtime-truth.md) | Deceptive `STABLE` log reporting on failed service initialization | **COMPLETED / HARDENED**. Segregated init lifecycle states (`DECLARED`, `SPAWNED`, `ENDPOINT_BOUND`, `READY`). Services spawn via genuine `bharat.process_manager` IPC. |
| [`embedded-board-linker-and-renode-review-2026-10.md`](embedded-board-linker-and-renode-review-2026-10.md) | Hardcoded `0x40010000` STM32 APB2 MMIO clash, RV32 OpenSBI S-mode vs bare-metal M-mode clash, missing compiler-rt division | **COMPLETED / HARDENED** (`BH-EMBED-LINK-P0-001`). Implemented declarative board manifests, parameterized linker generator, 10-point qualification CLI (`tools/board/validate.py`), compiler-rt division, and Nirmaan integration. |
| [`ipc-p0-002-authenticated-receive-envelope-review.md`](ipc-p0-002-authenticated-receive-envelope-review.md) | Kernel-authenticated sender provenance, 96-byte receive envelope, Syscall 9 selector dispatch, single capability role mapping | **COMPLETED / HARDENED** (`BH-IPC-P0-002`). Implemented versioned fixed-width envelope (`bh_ipc_receive_envelope_v1_t`), send-time provenance capture (`sender_pid`, `sender_process_object_id`, `sender_cspace_id`, `sender_owner_core`), dual-selector dispatch in Syscall 9, fault-safe copyout rollback, guest boot selftest (`ktest_ipc_envelope`), and 5-target QEMU qualification. |

---

## 2. Historical Audits & Baselines (Archived / Superseded)

These documents served as developmental milestones or gap analyses during early 2026 development. The architectural enhancements they proposed have since been integrated into the current architecture:

| Review Document | Original Date | Archival / Superseded Rationale |
|---|---|---|
| [`CODE_REVIEW_REPORT.md`](CODE_REVIEW_REPORT.md) | April 2026 | Early linter violations (`check_layer_references.py`). Layer references and Symbol leakages resolved in current tree. |
| [`architecture_authority_lifecycle_failure_review_2026-04-21.md`](architecture_authority_lifecycle_failure_review_2026-04-21.md) | 2026-04-21 | Addressed service authority lifecycles and fallback mechanisms. Superseded by ADR-021 through ADR-035. |
| [`architecture_review_enhancement_plan.md`](architecture_review_enhancement_plan.md) | April 2026 | Strategic enhancement plan for kernel/HAL boundaries. Integrated into [`AGENTS.md`](../../AGENTS.md). |
| [`code-quality-and-production-gap-analysis_2026-04-24.md`](code-quality-and-production-gap-analysis_2026-04-24.md) | 2026-04-24 | Initial production gap analysis. Most functional gaps (PMM, TLB, Syscalls) resolved in October 2026 baseline. |
| [`current-implementation-review.md`](current-implementation-review.md) | 2026-04-25 | Baseline review noting missing IDT/GIC/PLIC trap entries and runqueues. Both are now implemented. |
| [`gap_analysis_merged_review_2026-04-22.md`](gap_analysis_merged_review_2026-04-22.md) | 2026-04-22 | Merged gap analysis. Superseded by canonical target profiles in `delivery/targets/` and `tools/agents/verify.py`. |
| [`host-testing-and-qemu-runner-review_2026-04-21.md`](host-testing-and-qemu-runner-review_2026-04-21.md) | 2026-04-21 | Test harness review. Replaced by [`bharat-os-host-testing`](../../.agents/skills/bharat-os-host-testing/SKILL.md) and [`tools/agents/verify.py`](../../tools/agents/verify.py). |
| [`ipc_architecture_profile_readiness_review_2026-03-21.md`](ipc_architecture_profile_readiness_review_2026-03-21.md) | 2026-03-21 | Early IPC profile analysis. Superseded by fixed-width wire structures and capability-mediated endpoints. |
| [`kernel-production-readiness-audit-2026-08.md`](kernel-production-readiness-audit-2026-08.md) | 2026-08-08 | Comprehensive August 2026 readiness audit. Used as the baseline for the October 2026 stabilization effort. |
| [`kernel_algorithmic_foundations_code_mapping.md`](kernel_algorithmic_foundations_code_mapping.md) | April 2026 | Algorithmic code mapping. Algorithms verified or refactored to bounded structures. |
| [`main-branch-gap-validation_2026-04-21.md`](main-branch-gap-validation_2026-04-21.md) | 2026-04-21 | Early branch comparison. Superseded by `developer` convergence branch. |
| [`memory-and-fileop-layer-placement-review_2026-04-20.md`](memory-and-fileop-layer-placement-review_2026-04-20.md) | 2026-04-20 | VFS file operations and layer placement. Separated mechanisms in kernel and policy in `fd_mgr` / `ramfs`. |
| [`modern_primitives_personality_impact_review_2026-04-20.md`](modern_primitives_personality_impact_review_2026-04-20.md) | 2026-04-20 | Impact review of Linux/POSIX shims. Shims now strictly isolated in `core/personalities/`. |
| [`scaffold_todo_and_arch_separation_review_2026-04-22.md`](scaffold_todo_and_arch_separation_review_2026-04-22.md) | 2026-04-22 | First-pass inventory of stubs and TODOs. Superseded by active backlog in [`ROADMAP.md`](../../ROADMAP.md). |
| [`gap_analysis/`](gap_analysis/) | Spring 2026 | Historical directory of early ISA extensions, interrupt controller plans, and algorithm benchmarks. |

---

## 3. Active Investigations & Hardening Backlog

These reviews track open architectural investigations or hardening tasks actively targeted in current sprints:

| Review Document | Priority | Current Status & Target |
|---|---|---|
| [`CAP-P0-004-revocation-mapping-lifetime.md`](CAP-P0-004-revocation-mapping-lifetime.md) | **P0** | **Investigation complete; integration in progress.** Addresses capability revocation synchronization with active CPU page-table unmapping and device DMA unmapping. |
| [`automotive_build_hygiene_followups.md`](automotive_build_hygiene_followups.md) | **P1** | **Follow-up items.** Standardizing freestanding string headers and eliminating hardcoded personality tables from `syscall_gate.c`. |
| [`context-switching-hw-optimization-review.md`](context-switching-hw-optimization-review.md) | **P1** | **Partially aligned.** Priority bitmap bit-scan helpers implemented; hardware-telemetry informed slice shaping remains under development. |

---

## 4. Verification and Hygiene Standards

All reviews must adhere to the validation gates specified in [`AGENTS.md`](../../AGENTS.md):
- **Zero Falsified Evidence:** Never report unexecuted tests as `PASS`.
- **Automated Hygiene:** Run `python tools/lint/check_code_hygiene.py --check` before submitting any refactor.
- **Unified Validation:** Run `python tools/agents/verify.py --linters-only` to ensure architectural layer and agent configuration compliance.
