---
title: "Kernel production-readiness audit: five architectures, per-core kernel, profiles, and boot trust"
status: Audited
owner: Kernel Working Group
last_updated: 2026-08-08
tags:
  - architecture
  - audit
  - boot
  - memory
  - production-readiness
  - scheduler
---

# Kernel production-readiness audit (2026-08)

## 1. Decision and scope

**Decision: Bharat-OS is not production-ready on any of the five required
architectures today.** The repository contains a substantial architectural
baseline, but several security and correctness paths are scaffolds that report
success, the 32-bit ports are explicitly below baseline, and the required
profile matrix is not continuously demonstrated. `production_candidate: true`
in `delivery/targets/arch_maturity.yaml` must therefore be read as an intended
candidate tier, not a release qualification.

This audit covers:

1. x86_64, arm64, arm32, riscv32, and riscv64;
2. per-core kernel ownership and cross-core transactions;
3. MMU-full, MMU-lite, and MPU protection profiles;
4. GP, RT, and MIX scheduling profiles;
5. safe and fast boot, image loading, and firmware-to-kernel handoff; and
6. secure boot and controlled reboot.

The assessment is source-based. A target is **implemented** only when the
selected production path is non-stub, fails closed, and has executable negative
and fault-injection evidence. Compiling a file, defining a YAML profile, or
returning success from a placeholder is not implementation evidence.

## 2. Production release bar

A release candidate must satisfy all of the following. Failure of any P0 item
blocks production claims.

| Area | Required release evidence |
|---|---|
| Architecture | Boot, traps, interrupts, timer, context switch, user entry/return, fault-safe usercopy, syscalls, memory protection, SMP where supported, reboot, and crash capture execute on the named target. |
| Per-core ownership | No remote mutable pointer access; bounded versioned messages; owner/generation validation; idempotent replay; monotonic deadlines; targeted retry; rollback or quarantine; no remote wait under object locks. |
| Memory | Profile selection is singular and deterministic; runtime hardware validation rejects mismatches; map/protect/unmap and fault paths are real for the selected backend; MPU never pretends to provide paging; TLB completion is proven before reuse. |
| Scheduling | GP fairness and starvation bounds, RT admission/budget enforcement and bounded latency, and MIX partition/channel isolation are selected from the authoritative profile and tested under overload. |
| Boot | Every untrusted length, address, count, alignment, overlap, and arithmetic operation is validated before use; W^X and least privilege are established before runtime; loader and handoff ABIs are versioned and tested. |
| Trust | The complete boot chain is authenticated with real platform keys and anti-rollback; normal and recovery policy fail closed; debug/provisioning require explicit authorization; measurements are genuine and attestable. |
| Reboot | Capability-authorized userspace request, service quiesce, storage flush, secondary-core rendezvous, watchdog fallback, reason persistence, and cold/warm/emergency paths work on every target. |
| Assurance | Five required target smoke gates, all-architecture QEMU, host/unit tests, SMP stress, sanitizers/static analysis, fuzzing, fault injection, reproducible builds, signed artifacts, SBOM, and release provenance pass from a clean tree. |

## 3. Critical findings

### P0-1: secure boot currently permits unverified execution

`boot_security_evaluate()` defaults to `ALLOW`, permits an insecure recovery
boot with only a warning, and `boot_security_allows_mode()` permits debug and
provisioning modes. The architecture secure-boot implementations for arm64 and
riscv64 synthesize fixed digests, mark them verified, and return success; their
secure-memory and DMA-isolation operations are also success-returning
placeholders. Equivalent unsupported behavior is selected through the common
HAL where an architecture implementation is absent.

**Impact:** an unsigned, rolled-back, or modified kernel/service image can be
treated as trusted; a caller can also receive a false assertion that memory or
DMA isolation was established. This is a release-blocking trust-boundary flaw.

**Required closure:** introduce one versioned boot policy contract with required
key identifiers, minimum security version, allowed boot modes, recovery key,
debug authorization, measurement bank, and failure action. Verify signed
manifest metadata and every executable/configuration payload before parsing or
execution; bind hashes to type, length, load address, version, and profile;
enforce anti-rollback using platform monotonic storage; reject unavailable
security primitives for enforcing profiles. Add corrupted signature, wrong key,
truncation, reordered component, rollback, missing measurement, debug-bypass,
recovery-bypass, and power-loss update tests.

### P0-2: arm32 and riscv32 are bring-up ports, not production ports

The authoritative maturity file labels both 32-bit ports with partial boot,
stub traps, unsupported syscalls, scaffold MMU, unsupported SMP, and
`production_candidate: false`. Nevertheless, target YAML files expose MMU-lite,
MPU, RT, and MIX variants. A profile declaration does not close missing trap,
syscall, protection, and reboot paths; notably no arm32 `hal_cpu_reboot()`
implementation is present alongside the other four required architectures.

**Impact:** the five-architecture requirement is not met. User/kernel isolation,
fault recovery, preemption, and controlled reset cannot be claimed on edge32.

**Required closure:** complete architecture bring-up in this order: exception
vectors and frame ABI; interrupt/timer; fault-safe usercopy; generated native
syscall entry/return; context-switch and user transition; MMU-lite page-table
operations; MPU programming; cache/TLB maintenance; reboot/watchdog; then SMP
only on boards whose topology and interrupt hardware support it. Each primitive
needs positive and hostile input tests plus QEMU/board execution evidence.

### P0-3: boot policy and platform trust are not fail-closed

The common kernel entry does establish a useful staged order—early, security,
memory, platform services, runtime—but all non-BSP CPUs are routed to the same
secondary entry with a literal zero handoff argument. Target handoff metadata is
inconsistent: arm targets name a register, while several other protocols rely
on implicit conventions. The accepted boot-runtime ADR itself says behavior and
fault-injection validation are pending.

**Impact:** firmware data can be misinterpreted across protocols or lost on AP
startup; security failure may not stop memory/runtime initialization; loaders
cannot be qualified as a single robust chain.

**Required closure:** make `boot_info_t` a versioned, sized, immutable handoff
object copied into kernel-owned memory. Protocol adapters must normalize UEFI,
Multiboot2, FDT/Linux, OpenSBI, and U-Boot inputs through checked arithmetic and
range/overlap validation. Security must return an explicit decision consumed by
the boot state machine, with no transition to memory/runtime on denial. Define
per-ISA BSP/AP register contracts and preserve the real AP handoff. Enforce NX,
W^X, RELRO/read-only kernel data where available, guard pages, stack canaries,
KASLR entropy requirements for applicable profiles, and explicit unsupported
results elsewhere.

### P0-4: profile declarations are not yet one authoritative runtime contract

The generated configuration template contains separate device, kernel
execution, IRQ, memory, and multikernel switches. `system_profile.h` explicitly
records that legacy and new profile definitions remain ununified, while
`execution_mode.c` describes its compile definitions as mocks and falls back to
UNKNOWN modes and a single discovered CPU. Some target YAML `execution_profile`
values are not repeated in `cmake_defs`, creating reliance on build-tool
translation that must be proven rather than assumed.

**Impact:** the compiled kernel, advertised profile, runtime hardware, scheduler,
and memory backend can disagree. Silent fallback is unsafe for RT, MIX, MPU, and
secure profiles.

**Required closure:** generate a single, versioned profile descriptor from the
target schema. It must contain architecture, protection model, address-space
semantics, scheduler mode, CPU partition rules, boot/security policy, reboot
policy, required hardware capabilities, and enabled services. Add configure-time
mutual-exclusion/dependency checks and early runtime discovery validation.
Unknown, contradictory, or unavailable requirements must stop boot with bounded
diagnostics, not select a weaker default.

### P0-5: per-core ownership is accepted architecture but not fully qualified

The scheduler has owner-local runqueues and a detailed transactional migration
ADR. The memory ADR records TLB shootdown and hardware IOMMU paths as partial.
The implementation and documentation still contain transitional/scaffold code,
so statements such as “fully verified” cannot substitute for multi-ISA SMP,
queue saturation, replay, delayed ACK, stale generation, timeout, destroy race,
and partial-commit evidence.

**Impact:** a lost completion or stale ownership observation can create two
runnable instances, reuse memory before remote invalidation, leak a reservation,
or deadlock a core. These failures are rare and catastrophic—the exact class
that smoke boot tests will miss.

**Required closure:** inventory every mutable scheduler, PMM, VM, capability,
IPC, timer, and lifecycle object; record owner and legal transitions beside the
definition. Finish a common fixed-width wire envelope and bounded per-core
transport. Eliminate direct remote writes (including completion shortcuts unless
the shared-memory cell has a formally specified single-writer ownership and
memory-ordering contract). Use monotonic deadlines, targeted bounded retries,
replay caches, reconciliation of unknown outcomes, compensation, and quarantine.
Gate completion on static ownership checks and sustained SMP fault injection.

### P0-6: RT and MIX do not yet have production-grade temporal guarantees

The repository has real CFS and EDF/RMS structures, but the router only assigns
a global policy enum. EDF admission defaults to 100% utilization “for tests”,
RMS uses a simplified fixed 69% bound and coarse priority mapping, and execution
mode/partition initialization is described as scaffolding. There is no evidence
here of end-to-end worst-case interrupt, blocking, migration, IPC, and scheduler
latency budgets across all profile/backend combinations.

**Impact:** RT admission can accept an infeasible workload; GP work can interfere
with RT work in MIX; an apparent RT profile may provide no defensible deadline
guarantee.

**Required closure:** keep dispatch and budget mechanisms in the kernel and move
admission/orchestration policy to an authorized service. Define per-profile
scheduler-class availability, priority bands, CPU/IRQ partitions, runtime/period/
deadline units, replenishment, overrun response, priority inheritance ceilings,
and cross-partition channel budgets. Use overflow-safe fixed-width time math and
HAL monotonic units. Validate GP fairness/starvation, RT response-time/admission,
MIX overload isolation, IRQ interference, lock blocking, timer drift, and
migration with cyclictest-style latency distributions on hardware.

### P0-7: reboot is a panic hook, not a lifecycle protocol

Panic recovery is compile-time defaulted to halt; reboot is a direct HAL call.
There is no demonstrated capability-gated kernel reboot syscall, authorized
power-manager transaction, userspace quiesce protocol, storage durability
barrier, cross-core stop acknowledgement, reboot-reason persistence, or watchdog
fallback. Architecture implementations are uneven.

**Impact:** reboot can lose durable data, leave devices/DMA active, hang when a
core is wedged, or be invoked without an auditable authority path.

**Required closure:** define `RUNNING -> QUIESCING -> CORE_RENDEZVOUS -> RESETTING`
with an `EMERGENCY_RESET` escape. A service owns policy; the kernel validates a
reboot capability and provides bounded mechanisms. Quiesce services and block
new mutations, flush durable storage, mask/stop DMA, rendezvous cores without
holding subsystem locks, persist a fixed-width reason record, arm a watchdog,
and invoke the profile-selected reset backend. Test busy/lost cores, storage
failure, recursive panic, watchdog expiry, warm/cold reset, boot-loop limiting,
and recovery selection.

## 4. Capability matrix: evidence versus required state

| Target | Current evidence | Principal blockers | Production exit |
|---|---|---|---|
| x86_64 | Baseline boot/trap/syscall/MMU/SMP is declared; Multiboot2/Q35 target exists. | Secure boot is placeholder-grade; reset is minimal; SMP ownership, loader hardening, IOMMU, negative boot tests, and hardware qualification are incomplete. | Full GP/MMU/SMP suite, real UEFI/TPM or platform trust path, AP fault injection, VT-d where claimed, controlled reboot, and two representative hardware platforms. |
| arm64 | Baseline boot/trap/syscall/MMU and partial SMP are declared; Linux/FDT handoff exists. | Partial SMP; placeholder EL3/TrustZone verification and isolation; incomplete SMMU; boot handoff and PSCI reset need end-to-end qualification. | GIC/PSCI SMP stress, real verified-boot integration, SMMU isolation, GP/RT/MIX tests, reboot/panic, and two SoCs/boards. |
| riscv64 | Baseline boot/MMU and partial syscall/SMP are declared; OpenSBI target exists. | Syscall and SMP partial; fixed fake measurement accepted; PMP/IOPMP secure operations are placeholders; SBI version/extension variance is not qualified. | Generated syscall ABI, SBI capability gating, SMP/shootdown stress, real root-of-trust/PMP enforcement, reboot, and QEMU plus hardware. |
| arm32 | Partial boot, stub trap, unsupported syscall, scaffold MMU, no SMP; MMU-lite and MPU YAML exist. | Fundamental privilege, fault, isolation, reset, and user ABI paths incomplete. | Complete uniprocessor MMU-lite and MPU ports first; execute all negative memory/trap/syscall/reboot tests; add SMP only as a separate board capability. |
| riscv32 | Partial boot, stub trap, unsupported syscall, scaffold MMU, no SMP; MMU-lite and MPU YAML exist. | Fundamental privilege, fault, isolation, user ABI, PMP, and reset qualification incomplete. | Complete RV32 privilege/trap/syscall, Sv32 MMU-lite, PMP MPU, timer/PLIC, reset, and QEMU/hardware validation. |

## 5. Memory-profile findings

| Model | Present baseline | Production gap and fail-closed rule |
|---|---|---|
| MMU-full | Backend-neutral page-table and VM contracts exist for the 64-bit tier. | Prove map/protect/unmap atomicity, ASID/PCID lifecycle, W^X, user/kernel separation, TLB ACK-before-reuse, OOM rollback, huge-page alignment, COW/demand-fault races, and IOMMU independence. Never advertise IOMMU when the programming backend is a stub. |
| MMU-lite | Targets disable demand paging/COW/shared memory but currently set `BHARAT_ENABLE_ADVANCED_VM: ON`, an ambiguity that should be eliminated by generated capability rules. | Define the exact supported operation set; reject rich VM requests with `K_ERR_UNSUPPORTED`; prove Sv32/ARM short-descriptor permission and cache/TLB behavior, bounded metadata, no allocation in RT fault paths, and 32-bit overflow safety. |
| MPU | arm32/riscv32 RT targets exist and select region-only intent. | Prove actual region programming, default-deny background region, executable/write separation, priority/overlap semantics, context-switch reprogramming bounds, region exhaustion, alignment/rounding, stack guards, and rollback. No page API may return success on MPU. |

Required cross-product qualification is not every theoretical combination. It is
the set emitted by the authoritative target matrix, with at least: MMU-full + GP
and MIX on each 64-bit ISA; MMU-lite + GP/MIX on both 32-bit ISAs; MPU + RT on
both 32-bit ISAs; and explicit configure-time rejection tests for all prohibited
combinations.

## 6. Prioritized implementation plan

### Phase 0 — stop unsafe claims and lock contracts (weeks 0–4)

1. Change maturity reporting so “candidate” cannot be confused with qualified;
   publish executable evidence links and expiry dates.
2. Make enforcing secure-boot profiles deny placeholder/unsupported backends.
3. Define authoritative schemas for the target/profile descriptor, boot manifest
   and handoff object, reboot transaction/reason, and cross-core wire envelope.
4. Add CI schema validation and forbidden-placeholder checks for production
   target closures.
5. Build a requirement-to-test traceability matrix with stable IDs for every row
   in section 2.

**Exit:** no false-success security path; contradictory profiles fail configure
or early boot; every production claim names reproducible evidence.

### Phase 1 — five-ISA architectural correctness (months 1–3)

1. Finish arm32 and riscv32 trap, timer/interrupt, context, usercopy, generated
   syscall, user transition, cache/TLB, and reset paths.
2. Close riscv64 syscall and arm64/riscv64 SMP gaps.
3. Add architecture conformance tests that share one semantic suite but use
   ISA-specific frame/register assertions.
4. Run QEMU on every commit and real hardware nightly; record firmware, QEMU,
   compiler, linker, and board revisions.

**Exit:** all five targets boot userspace, execute and reject syscalls correctly,
survive faults, preempt, protect memory, and reboot from clean builds.

### Phase 2 — memory and per-core correctness (months 2–5)

1. Finish MMU-full, MMU-lite, and MPU operations behind the authoritative MPA
   interface; remove or isolate legacy bypasses.
2. Complete owner-local PMM magazines, VM mutation serialization, targeted TLB
   transactions, capability owner/generation handling, and bounded uRPC.
3. Add model-checkable state machines for migration, shootdown, revoke, destroy,
   and reboot; add C static assertions for every wire size/offset.
4. Stress duplicates, delay, loss, reordering, queue full, generation wrap,
   timeout, partial ACK, hotplug/offline, and concurrent destruction.

**Exit:** no shared remote mutation; all unknown outcomes reconcile; no frame or
entity is reused before ownership and invalidation completion; all three memory
models pass lifecycle, isolation, rollback, and saturation suites.

### Phase 3 — GP/RT/MIX guarantees (months 4–7)

1. Replace global/profile ambiguity with per-core scheduler configuration and an
   immutable partition plan validated against discovered topology.
2. Implement bounded RT replenishment, admission inputs, overrun handling,
   priority inheritance, and timer semantics; keep policy in a capability-bound
   manager service.
3. Enforce MIX CPU, IRQ, memory, and IPC budgets; deny cross-partition channels
   not granted by capability.
4. Establish latency budgets per supported board/profile and collect p50/p99/
   p99.9/max under interrupt, memory, IPC, network, and storage stress.

**Exit:** GP fairness and throughput regressions are bounded; RT deadlines meet
the declared hardware-specific envelope; GP overload cannot violate MIX RT
budgets; results are reproducible over long-duration tests.

### Phase 4 — verified boot, loading, handoff, update, and reboot (months 5–8)

1. Implement signed manifests and streaming authenticated loading with checked
   bounds, overlap, decompression ratios, relocations, and W^X transitions.
2. Integrate UEFI Secure Boot/TPM on x86_64, an authenticated EL3/firmware chain
   on arm64, and a board-specific immutable root/OpenSBI measurement path on
   RISC-V. Treat unavailable roots as unsupported, never verified.
3. Add A/B transactional update, anti-rollback, boot-attempt counters, confirmed
   health promotion, recovery authorization, and power-loss safety.
4. Implement the capability-mediated reboot lifecycle and platform/watchdog
   backends on all five targets.
5. Measure and budget each boot stage. Optimize only after correctness: parallel
   signature hashing/I/O where safe, avoid redundant copies, initialize optional
   services after the trusted runtime milestone, and preserve deterministic RT
   startup ordering.

**Exit:** hostile boot corpus cannot cross the trust boundary; every loaded byte
is authenticated before execution; rollback and debug bypass are rejected;
failed updates recover; clean and emergency reboot work with auditable reasons.

### Phase 5 — assurance and release qualification (months 7–12)

1. Enable warnings-as-errors for supported toolchains, clang-tidy/Coverity-class
   analysis, UBSan-hosted tests, fuzzers for parsers/syscalls/IPC, race-oriented
   stress, and coverage reporting.
2. Produce reproducible signed release artifacts, SBOM, provenance, vulnerability
   response policy, key-rotation ceremony, and recovery/key-revocation runbooks.
3. Execute 72-hour SMP/IO stress, reboot-loop, brownout/update, memory-pressure,
   RT overload, and fault-injection campaigns per release target.
4. Conduct independent security review and architecture-specific penetration
   testing; resolve every P0/P1 or formally remove the affected claim/profile.

**Exit:** all section 2 gates pass twice from clean infrastructure; evidence is
reviewed, immutable, and tied to the exact source/toolchain/artifacts.

## 7. Required validation matrix

The minimum continuous matrix is:

| Axis | Required values |
|---|---|
| ISA | x86_64, arm64, arm32, riscv64, riscv32 |
| Protection | MMU-full (64-bit), MMU-lite (32-bit and declared 64-bit variants), MPU (arm32/riscv32) |
| Scheduler | GP, RT, MIX where the target descriptor allows it; rejection for forbidden combinations |
| CPU | UP for all; SMP for x86_64/arm64/riscv64 and any explicitly SMP-capable 32-bit board |
| Boot | normal, recovery, update, corrupted image, rollback, missing firmware data, malformed handoff |
| Reset | clean warm/cold, emergency/panic, watchdog, lost secondary core, repeated boot failure |

Every run must emit machine-readable evidence containing source revision, dirty
state, target descriptor digest, compiler/linker/emulator or board firmware
versions, selected profile digest, discovered hardware capabilities, security
decision, test IDs, duration, and result. A skipped row is BLOCKED, not PASS.

## 8. Architecture and security invariants for implementation

* Mutable state has one owner core; ownership transfer is generation-checked and
  commits exactly once.
* Cross-core structures are fixed-width, versioned, bounded, pointer-free, and
  compile-time layout asserted.
* No runqueue, address-space, capability-table, PMM, or object lock is held while
  waiting for another core.
* Timeouts use monotonic HAL time; retry is bounded and targets only missing
  participants; replay is idempotent.
* Destruction enters `DYING`; new mutations are rejected; irreconcilable partial
  failure quarantines the object.
* Hardware truth comes from HAL/platform discovery and is checked against, not
  invented by, the compile-time profile.
* Unsupported protection, trust, scheduler, loader, or reset operations fail
  closed with a canonical status.
* Capability validation covers object type, rights, scope, owner, generation,
  liveness, and post-usercopy embedded handles.
* No executable content runs before authentication and policy acceptance; no
  writable mapping remains executable after relocation.

## 9. Documentation impact and audit limitations

This audit changes no architecture contract or implementation. It consolidates
the existing accepted direction into a production gate and deliberately treats
unexecuted or placeholder behavior as incomplete. No generated configuration,
ABI, third-party source, or target behavior is changed.

The audit does not certify functional safety, hard real-time behavior, or secure
boot. Those require hardware-specific timing, threat modeling, key management,
independent review, and executable evidence described above. Until those gates
exist, downstream product profiles must describe Bharat-OS as development or
experimental rather than production-grade.
