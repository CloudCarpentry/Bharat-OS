# Bharat-OS Convergence & Hardening Roadmap

**Repository Baseline**: Commit `3041933a` (October 14, 2026)  
**Constitutional Authority**: [AGENTS.md](AGENTS.md)  
**Machine-Readable Maturity**: [interface/contracts/implementation_maturity.json](interface/contracts/implementation_maturity.json)

---

## 1. Architectural Strategy & Ground Rules

Bharat-OS has progressed substantially beyond early scaffolding. ARM32 MPU/RV32 PMP, MMU-Lite hardening, HMEM, capability revocation, Linux compatibility shims, Android/Binder personality, devmgr, TLB shootdown protocols, and PMM per-core caches (`pmm_pcache`) now exist in the tree.

**Strategic Principle**:
> **Stop treating obsolete "missing feature" roadmaps as authoritative. Focus entirely on runtime proof, hardening, failure semantics, and removing architectural drift.**

Milestones require the **Three-Part Truth Standard** defined in [`bharat-os-runtime-truth`](.agents/skills/bharat-os-runtime-truth/SKILL.md):
1. **Source Truth**: Mechanism in correct layer (`core/kernel/`), policy in services (`core/services/`).
2. **Build Truth**: Fully wired into CMake and active target configurations.
3. **Execution Truth**: Production authority adapters installed, automated regression tests pass, and canonical QEMU runtime markers are observed.

---

## 2. Active Engineering Backlog

### P0 Wave: Runtime Blockers & Architectural Invariants

| Ticket ID | Priority | Subsystem | Description & Acceptance Criteria | Status |
|---|---|---|---|---|
| **`BH-AGENT-P0-005`** | P0 | Tooling / Agents | Consolidate agent skills into canonical `.agents/skills/`, mirror to `.claude/skills` & `.github/skills`, streamline `AGENTS.md` into 4 pillars. | **DONE** |
| **`BH-TRUTH-P0-006`** | P0 | Tooling / Truth | Build `tools/agents/context_pack.py`, `context_map.yaml`, `verify.py`, and `check_agent_config.py`. | **DONE** |
| **`BH-VM-P0-001`** | P0 | `vm_manager` | **Native VM Authority Adapter**: Implement `core/services/vm_manager/kernel_adapter.c`. Bind `vm_manager` to canonical kernel VM mechanisms (`aspace -> region -> vm_object`). Must emit `VM_MANAGER_AUTHORITY_READY` in QEMU. | **IN PROGRESS** |
| **`BH-RUNTIME-P0-002`** | P0 | Service Runtime | **Boot Lifecycle Gate**: Init -> namesvc -> processmgr -> vmmgr -> servicemgr E2E boot proof emitting all readiness markers. | QUEUED |
| **`BH-SCHED-P0-003`** | P0 | Scheduler / Time | **Monotonic Bounded Waits**: Replace iteration-counter loop `sched_wait_remote_cmd_ack(..., timeout_loops)` with monotonic HAL time (`bh_ktime_now()`). | QUEUED |
| **`BH-CAP-P0-004`** | P0 | Capabilities | **IPC Authorization Coverage**: Implement static/runtime coverage linter verifying every IPC dispatch operation enforces capability authority. | QUEUED |
| **`BH-PERF-P0-001`** | P0 | Performance / IPC | **Multikernel Concurrency & Latency Proof**: Measure local fast path vs shared-memory fast path vs cross-core control path costs under multi-core load. Eliminate synchronization bottlenecks and verify ADR-039 three-tier concurrency model. | PLANNED |

---

### P1 Wave: Concurrency Hardening & Profile Proof

| Ticket ID | Priority | Subsystem | Description & Acceptance Criteria | Status |
|---|---|---|---|---|
| **`BH-TLB-P1-001`** | P1 | TLB / MM | Decouple TLB shootdown from scheduler `g_active_core_count` to ensure strict layer boundary. | PLANNED |
| **`BH-PROC-P1-002`** | P1 | `process_manager` | Real ELF filesystem -> process -> VM -> scheduler E2E verification under QEMU. | PLANNED |
| **`BH-CAP-P1-003`** | P1 | Capabilities | Deep/wide revocation, generation rollover, retry, and replay saturation stress matrix. | PLANNED |
| **`BH-PMM-P1-004`** | P1 | PMM | Per-core magazine (`pmm_pcache`) and remote-free queue saturation & failure testing under heavy multi-core load. | PLANNED |
| **`BH-NET-P1-005`** | P1 | `netmgr` | Replace remaining polling scaffolding with genuinely event-driven receive dispatch. | PLANNED |
| **`BH-LINUX-P1-006`** | P1 | Personalities | Linux compatibility test suite verifying epoll, eventfd, timerfd, futex, and pipe shims. | PLANNED |
| **`BH-ANDROID-P1-007`** | P1 | Personalities | Android personality & Binder IPC end-to-end verification. | PLANNED |
| **`BH-EDGE32-P1-008`** | P1 | Arch (32-bit) | ARM32 MPU and RISC-V 32 PMP fault isolation, privilege boundary, and negative test matrix. | PLANNED |
| **`BH-GUI-P1-009`** | P1 | UI / Experience | ADR-035 display tier conformance (Tier 0 Headless to Tier 3 Composited Desktop) and input stream validation across Desktop, Mobile, and Automotive profiles. | PLANNED |

---

### P2 Wave: Scaffold Elimination & Cleanliness

| Ticket ID | Priority | Subsystem | Description & Acceptance Criteria | Status |
|---|---|---|---|---|
| **`BH-SVC-P2-001`** | P2 | `coremgr` | Replace dummy endpoint handle with runtime-created capability endpoint. | BACKLOG |
| **`BH-SVC-P2-002`** | P2 | `faultmgr` | Connect stub fault manager to supervisor and coremgr. | BACKLOG |
| **`BH-SVC-P2-003`** | P2 | `memmgr` | Implement fault queue and COW memory policy in userland after VM authority stabilizes. | BACKLOG |
| **`BH-LINT-P2-004`** | P2 | Lint / Quality | Establish baseline linter for mid-file includes, naming conventions, and stale TODO tracking. | BACKLOG |

---

## 3. Mandatory Qualification Matrix

Every ticket moving from PLANNED to DONE must pass:
```bash
# 1. Agent configuration and layer reference linters
python3 tools/agents/verify.py --changed-files <files>

# 2. Syscall ABI lock check (if ABI touched)
python3 tools/abi/syscall_abi.py --check

# 3. Canonical 5-Target QEMU Smoke Matrix Gate
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```
