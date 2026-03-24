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
| Kernel boundary + syscall ABI freeze | **Partial** | Canonical UAPI and syscall status translation path are in active convergence; remaining work includes eliminating legacy ad-hoc negative subsystem returns, adding ABI drift CI gates, and BIDL contract status conformance checks. |

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

## B) Discrepancy log (explicit)

| Area | Discrepancy | Action |
| --- | --- | --- |
| Service status wording | “Implemented” may overstate runtime readiness for some services. | Keep architecture intent, but classify code status using taxonomy labels. |
| Capability mediation claims | Some code paths still use permissive/stub checks. | Mark as partial enforcement and track hardening milestones. |
| “Current phase” interpretation | Readers may assume production depth from phase labels. | Add per-item maturity labels and evidence links. |

## C) Deviation policy

When implementation deviates from architecture intent:

1. Document deviation in `docs/current-code-status.md` with taxonomy label.
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
