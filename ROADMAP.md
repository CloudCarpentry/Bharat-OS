# Bharat-OS Evolution Roadmap

This roadmap tracks Bharat-OS from a bootable microkernel baseline toward a production-hardened distributed multikernel. It is intentionally both:

- **Reality-backed** (what is currently in tree), and
- **Forward-looking** (what we are actively converging toward).

## Maturity taxonomy (applies across this roadmap)

- **Scaffold**: Buildable skeleton, placeholders, or TODO loops.
- **Partial**: Concrete logic exists for one or more core paths, but not end-to-end hardened.
- **Baseline**: End-to-end path exists for core scenarios and is usable for integration/developer workflows.
- **Production**: Hardened behavior, security depth, observability, stress validation, and operational runbooks.

> Rule for updates: any roadmap item must carry one of the four labels above and should point to code/docs evidence when possible.

## Phase 0V: Verification spine, concurrent with core stabilization (P0)
| ID | Deliverable | Dependency | Acceptance/evidence |
|---|---|---|---|
| VER-001 | Exact seL4-vs-Bharat capability semantic matrix; frozen definition of authority, delegation, revocation and linearization | Existing capability contract | ADR reviewed; non-equivalence and TCB listed |
| VER-002 | TLC-checked capability lifecycle model | VER-001 | No attenuation escalation/stale use/double commit in configured bounded state space; traceability to cap code |
| VER-003 | TLC-checked IPC capability transfer and queued-message model | VER-001 | lost/duplicate/delayed/failed receive and revoke races explored; negative C tests |
| VER-004 | TLC-checked shootdown request/ACK/failure model | Existing TLB contract | No premature frame reuse, ACK correlation, timeouts/poison semantics |
| SEC-001 | Audit cap validation and requester identity, remote mutation, transaction allocation failure | VER-001 | fail-closed tests; review of owner-only mutation and privileged PID 0 contract |
| SEC-002 | Security claim CI gate + evidence matrix | VER-002/003/004 | Each claim identifies config, tests/proof and limitation; no unsupported 'verified' claims |

## Phase 1S: Software baseline and actual runtime enforcement (P0/P1)
- Complete service-boundary authorization coverage and IPC transfer/revocation stress tests.
- Harden allocator (canary/redzone, quarantine, generations) without requiring optional hardware.
- Ensure ELF process/vm manager and service supervisor provide a genuine demo; keep existing baseline milestones as release blockers.
- Add CBMC/Frama-C bounded contracts for selected C paths where feasible; measure proof coverage, not just number of tools.

## Phase 2H: Optional hardware hardening (P2, no effect on unsupported architectures)
- HAL granular normalized MTE/CHERI-related *discovery* and opt-in enablement, with software baseline and target-specific unsupported return codes.
- ARM64 MTE isolated pilot with CPU/firmware checks and tagged allocator/fault tests.
- CHERI-RISC-V feasibility spike in a separate experimental toolchain/ABI image, explicitly not a generic riscv64 release requirement.
- Add DMA/IOMMU capability boundary tests before treating tagged pointers as device access controls.

## Phase 3T: Trusted lifecycle (P2/P3)
- Signed OTA manifest with algorithm/version/key-id, signature coverage, trust-anchor/rotation and anti-rollback; ML-DSA exploratory test vector first; legacy signature path retained by policy.
- Measured boot evidence accurate to platform trust chain, with explicit unsupported/not-measured/not-verified states.
- Confidential guest prototype target-by-target (SEV-SNP, TDX, CCA, CoVE), attestation and threat model; zero default 'TEE secure' claims.
- Rust for new *isolated services/drivers* only after ABI/error/lifetime contract; pilot `unsafe` inventory, no kernel rewrite.

## Deferred / forbidden shortcut
- Full kernel functional correctness certification, all-profile seL4-level parity, blanket CHERI or MTE, confidential host/hypervisor, universal PQ ROM boot are not P0 demo objectives.
- Reject silent software fallback with reported hardware protection, hardcoding trusted measurements, or moving domain policy into kernel/HAL.

## Release gates / owners (suggested)
G0: every new sensitive operation documented with owner, principal and capability; G1: TLC CI for three core models plus counterexample archive; G2: negative tests against implementation and all applicable target builds; G3: hardware enhancements only where probes, runtime tests and evidence are green; G4: architecture review ensures external user ABI freeze. Assign parallel agents: A cap semantics/model; B IPC model/tests; C TLB model/tests; D HAL/ISA feature inventory; E boot/update policy and evidence CI. Agents A-C may work independently until protocol contract freeze, then integrate.

---

## Phase 1: Core multikernel foundation (current focus)

| Item | Current maturity | Notes |
| --- | --- | --- |
| Per-CPU state management (runqueues/cap tables/memory shards) | **Baseline** | Core direction is active; hardening remains profile-dependent. |
| Multicore bootstrap + monitor processes | **Partial** | Control-plane and lifecycle behavior still being hardened. |
| Dynamic lockless URPC channels | **Baseline** | Primitive exists; reliability/backpressure hardening remains. |
| Cross-core capability operations | **Partial** | Delegation/revocation path exists, but lifecycle proofs and temporal controls remain. |
| Message-based TLB shootdown | **Partial** | Functional path exists; ack/completion semantics and stronger stress validation are still open. |
| Basic SKB/topology discovery | **Partial** | Present as baseline subsystem direction; depth varies by architecture. |
| Bootstrap control-plane split (`init` → `servicemgr` → `policymgr`) | **Partial** | Direction is now explicit: `init` remains a thin trusted bootstrap layer, while long-lived lifecycle policy converges into `servicemgr` and profile-aware policy managers. Remaining work is completion of supervision/event-loop depth and profile contracts. |
| Kernel boundary + syscall ABI freeze | **Partial** | Canonical UAPI and syscall status translation path are in active convergence; remaining work includes eliminating legacy ad-hoc negative subsystem returns, adding ABI drift CI gates, and BIDL contract status conformance checks. |
| Production-grade Interrupt Architecture | **Partial** | Core architecture unified (`hal_irq_` flow), but full `irq_domain` resolution, hardware specific mapping depth (e.g. MSI-X), and strict domain-first routing enforcement remain to be implemented. Documentation is in place. |

## Phase 2: Device specialization & edge UI

| Item | Current maturity | Notes |
| --- | --- | --- |
| Subsystem isolation for non-essential drivers | **Partial** | Direction in place; maturity differs by subsystem/service. |
| Secure boot + OTA validation | **Scaffold** | Security policy hooks exist; full measured/attested chain remains roadmap. |
| Framebuffer + input subsystem | **Partial** | Boot/display and framebuffer paths exist; full production UX stack remains deferred. |
| Deterministic AI scheduling heuristics | **Scaffold** | Telemetry/hook direction exists; strict boundedness/admission hardening required. |

## Phase 3: Cloud, accelerators, datacenter

| Item | Current maturity | Notes |
| --- | --- | --- |
| NUMA-aware demand paging | **Partial** | NUMA-aware APIs exist; policy depth and validation not yet production level. |
| Heterogeneous accelerators (DMA/NPU/GPU) | **Scaffold** | Manager scaffolding exists; concrete end-to-end accelerator pipelines are open. |
| High-speed networking (full TCP/IP + bypass paths) | **Partial** | Net split has substantial logic; full TCP maturity and fast paths remain open. |
| Scale-out multikernel messaging (cross-node) | **Scaffold** | Local multikernel messaging exists; cross-node fabric transport remains roadmap. |

## Phase 4: Advanced UX & verified core

| Item | Current maturity | Notes |
| --- | --- | --- |
| Hardware-accelerated compositor | **Scaffold** | Planned progression from framebuffer-first strategy. |
| Isabelle/HOL proof foundations | **Scaffold** | Verification scope documented, formal chain not yet integrated. |
| Linux/Android personality maturity | **Partial** | Contracts/architecture documented; end-to-end compatibility depth is ongoing. |
| Epic E3-X: Kernel Algorithmic Foundations | **Scaffold** | Cross-cutting foundation for scalable VM, object indexing, and RCU. See `latest_gap_analysis.md`. |

## Phase 5: Image release automation and distribution (future plan)

| Item | Current maturity | Notes |
| --- | --- | --- |
| Multi-board image build matrix in GitHub Actions | **Scaffold** | Add tag-triggered workflow (`v*`) that builds per-board images in parallel (for example: `raspberrypi4`, `orangepizero`, `generic-arm64`). |
| Image post-processing and compression | **Scaffold** | Standardize output naming (`bharat-os-<board>.img.xz`), add optional image shrink/minimize step, and publish checksums. |
| GitHub Release asset publishing | **Scaffold** | Automate release creation from CI with uploaded `.img.xz` artifacts and board-wise release notes. |
| Provenance and signing for downloadable images | **Scaffold** | Extend release pipeline to sign image assets and publish detached signatures with manifest hash links. |
| Runner disk-space hardening for large OS builds | **Scaffold** | Add deterministic cleanup strategy on runners and optional larger/self-hosted runners when image builds exceed hosted limits. |

---

## Full gap-analysis pack

## A) Reality vs intent (high-level)

1. **Kernel primitives lead service maturity**: core architectural direction is stronger than many user-space daemon runtime loops.
2. **Networking is ahead of most services**: `netmgr`/`netstack` have meaningful internal modules, while many other managers are stubs.
3. **Security depth gap**: capability and policy structure exists, but enforcement depth (e.g., full cap checks, IOMMU depth, verified boot chain) still needs sustained implementation.
4. **Observability gap**: baseline diagnostics exist, but production-grade trace/metrics/export and watchdog policy coverage are not fully closed.
5. **Lifecycle authority migration gap**: architecture direction is clear, but full migration from early-boot bootstrap logic into durable `servicemgr`/policy-manager ownership is still in progress.
6. **Production gate gap**: capability mediation is not uniformly strict across all manager paths yet; until this is closed, production-readiness claims remain blocked.

## B) Discrepancy log (explicit)

| Area | Discrepancy | Action |
| --- | --- | --- |
| Service status wording | “Implemented” may overstate runtime readiness for some services. | Keep architecture intent, but classify code status using taxonomy labels. |
| Capability mediation claims | `netmgr` now uses fail-closed validation, but capability mediation is still not complete across all manager/dispatch paths. | Keep mediation at **Partial**, track removal of remaining permissive/stub checks, and block production claims until strict enforcement is end-to-end. |
| “Current phase” interpretation | Readers may assume production depth from phase labels. | Add per-item maturity labels and evidence links. |

## C) Deviation policy

When implementation deviates from architecture intent:

1. Document deviation in `docs/dev/current-code-status.md` with taxonomy label.
2. Add roadmap closure item (owner + target phase).
3. Keep architecture docs forward-looking, but mark current maturity and constraints.

## D) Parallel execution policy (“small but solid”)

- Teams/agents can execute in parallel by subsystem or kernel area.
- Prefer small, isolated vertical slices that are testable (no “big bang” merges).
- A change is considered “solid” when it has:
  - clear scope,
  - bounded interfaces,
  - at least one validation path (build/test/check), and
  - documented maturity/status impact.

---

## Traceability update process

For each roadmap item, maintain:

- **Owner area** (kernel/mm/ipc/net/service/build),
- **Current maturity** (Scaffold/Partial/Baseline/Production),
- **Evidence** (files/tests/docs),
- **Next smallest solid milestone**.

This avoids roadmap drift and supports parallel development without status inflation.

---

## Build system governance requirement (CMake + agents/developers)

All roadmap execution must align with the build governance defined in:

- [`docs/architecture/cmake-governance-and-agent-rules.md`](docs/architecture/cmake-governance-and-agent-rules.md)

This document defines CMake structure, versioning expectations, and required contributor/agent behavior for adding or changing targets.


## Component architecture references

For per-domain architecture decomposition (with Mermaid + PlantUML diagrams, done/todo status, and roadmap mapping), see:

- [`docs/architecture/memory/roadmap.md`](docs/architecture/memory/roadmap.md)
- [`docs/architecture/components/kernel-subcomponents-architecture.md`](docs/architecture/components/kernel-subcomponents-architecture.md)
- [`docs/architecture/components/subsystem-subcomponents-architecture.md`](docs/architecture/components/subsystem-subcomponents-architecture.md)
- [`docs/architecture/components/services-subcomponents-architecture.md`](docs/architecture/components/services-subcomponents-architecture.md)
- [`docs/architecture/components/drivers-subcomponents-architecture.md`](docs/architecture/components/drivers-subcomponents-architecture.md)
