# Bharat-OS Repository Agent Constitution

This file is the authoritative, repository-wide constitution for autonomous and interactive coding agents working on Bharat-OS (including Google Jules, Google Antigravity, GitHub Copilot, Claude, and Gemini Code Assist).

All agents must strictly obey the four sections of this constitution. Procedural details, checklists, and domain procedures reside in the canonical skills under `.agents/skills/`.

---

## 1. Architectural Constitution

Bharat-OS is a verification-oriented, capability-based microkernel evolving through three compatible phases:
1. **Core Microkernel**: A minimal trusted computing base containing deterministic mechanisms only.
2. **Per-Core Kernel**: Core-local ownership, local scheduling, and explicit cross-core message coordination.
3. **Distributed Kernel**: Message-based distributed object lifecycle and ownership across cores and nodes.

### Non-Negotiable Architectural Principles
- **Mechanisms vs. Policy**: The kernel owns deterministic mechanisms only; userland services own policy, resource allocation, and orchestration.
- **Capability-Mediated**: Every resource access is mediated by an unforgeable capability token. Deny by default, fail closed.
- **Strict Ownership**: Every mutable kernel object has a single documented owner core. No shared mutable cross-core state without an explicit message-based ownership protocol.
- **Backend Neutrality**: Support MMU_FULL, MMU_LITE, and MPU/PMP profiles through backend-neutral abstractions (`aspace -> region -> vm_object`). Fail closed (`BH_STATUS_ERR_UNSUPPORTED`) if a target lacks required hardware.
- **Discovery**: Dynamic hardware facts (RAM size, core count, peripherals) are discovered via HAL/device-tree at runtime, never hardcoded in compile-time configuration.
- **Layer & Folder Boundaries**:
  - `core/kernel/`: Scheduling primitives, memory mapping mechanisms, capability translation, trap entry, IPC/uRPC delivery only.
  - `core/arch/`: ISA-specific assembly and CPU register control.
  - `core/hal/`: Architecture-neutral hardware contracts and timer/interrupt glue.
  - `platform/`: Machine, SoC, topology, and board wiring.
  - `core/drivers/`: Hardware control and device mechanisms (no kernel policy).
  - `core/services/`: Userland system services (vm_manager, process_manager, servicemgr, netmgr, devmgr).
  - `core/personalities/`: Foreign compatibility translation shims (Linux, Android, POSIX) strictly outside the kernel.
  - `interface/`: Stable external contracts, UAPI, IDL, and syscall ABI authorities.
  - `quality/`: Tests, stress suites, benchmarks, and verification evidence.
  - `delivery/`: Target YAML definitions, profiles, and packaging.
  - `tools/`: Build automation, linters, generators, and agent verification tools.
- **Naming Conventions**:
  - Public kernel functions: `bh_<subsystem>_<operation>()`
  - Public types: `bh_<name>_t`
  - Constants, macros, and enums: `BH_<SUBSYSTEM>_<NAME>`

---

## 2. Absolute Prohibitions

The following actions are strictly prohibited in all contributions:
1. **No Global Mutable Shortcuts**: Never introduce global mutable variables or reach into remote runqueues to avoid designing proper ownership.
2. **No Direct Remote Core Mutation**: Cross-core actions must use bounded, message-based protocols.
3. **No Pointer Crossing Across Wire Boundaries**: Never pass raw pointers, stack addresses, allocator addresses, function pointers, or architecture-dependent `long`/`size_t` across IPC, uRPC, or wire structures. Wire structs must be fixed-width with compile-time `static_assert` on size and alignment.
4. **No Spin-Count Waits**: Prohibit spin loops with iteration counters (e.g., `for (int i=0; i<timeout_loops; i++) arch_cpu_relax();`). All remote waits and deadlines must use monotonic HAL time (`bh_ktime_now()`).
5. **No Holding Locks Across Remote Waits**: Never hold an address-space, scheduler, capability-table, or object lock while waiting for remote completion.
6. **No Capability Bypasses**: Never bypass `interface/contracts/abi/native_syscalls.json` or capability validation with synthetic or raw numerical handles.
7. **No Direct User Pointer Dereference**: Never dereference user pointers before fault-safe usercopy (`bh_usercopy_from_user()`). Capability handles embedded in user structures must only be validated in the kernel copy (TOCTOU prevention).
8. **No Committing Generated Files**: Never commit generated build artifacts or configuration headers (`bharat_config.h`, generated syscall tables). Modify `core/kernel/include/bharat_config.h.in` for configuration changes.
9. **No Third-Party Alterations**: Never touch `ext/` or `external/` unless the task explicitly commands a reviewed update.
10. **No Falsified Evidence**: Never report unexecuted, missing, or skipped tests as PASS. If a dependency, toolchain, or emulator is unavailable, mark as BLOCKED.

---

## 3. Skill Routing Table

Do NOT load or scan the entire repository context. Use `tools/agents/context_pack.py` and route directly to the canonical skill in `.agents/skills/`:

| Task Subsystems / Keywords | Canonical Skill | Trigger & Scope |
|---|---|---|
| **Any non-trivial task entry** | [`bharat-os-task-router`](file:///.agents/skills/bharat-os-task-router/SKILL.md) | Task classification, context packing, and minimal skill selection |
| **Kernel mechanisms, HAL, drivers** | [`bharat-os-kernel-architecture`](file:///.agents/skills/bharat-os-kernel-architecture/SKILL.md) | Layer placement, mechanism-vs-policy, ownership |
| **Scheduler, cross-core, TLB, uRPC** | [`bharat-os-distributed-kernel`](file:///.agents/skills/bharat-os-distributed-kernel/SKILL.md) | Core-local state, monotonic deadlines, wire structs, replay |
| **VM, PMM, MMU, MPU, PMP, DMA** | [`bharat-os-memory-model`](file:///.agents/skills/bharat-os-memory-model/SKILL.md) | Tri-model memory, pcache magazines, no recursive memops |
| **Capabilities, CSpace, security** | [`bharat-os-capability-security`](file:///.agents/skills/bharat-os-capability-security/SKILL.md) | Rights/type/generation checks, revocation, usercopy phase |
| **Services, init, vm/process managers** | [`bharat-os-service-runtime`](file:///.agents/skills/bharat-os-service-runtime/SKILL.md) | Production authority adapters, real endpoints, runtime markers |
| **Display, GUI, touch, input stream** | [`bharat-os-gui-experience`](file:///.agents/skills/bharat-os-gui-experience/SKILL.md) | ADR-035 display tiers, ADR-034 input streams, visual smoke tests |
| **Host unit/integration testing** | [`bharat-os-host-testing`](file:///.agents/skills/bharat-os-host-testing/SKILL.md) | Fast CTest test suites, mock HAL, synthetic core concurrency |
| **CMake, profiles, target switches** | [`bharat-os-kernel-config`](file:///.agents/skills/bharat-os-kernel-config/SKILL.md) | Template authority (`bharat_config.h.in`), deterministic builds |
| **Syscalls, UAPI, native ABI** | [`bharat-os-syscall-abi`](file:///.agents/skills/bharat-os-syscall-abi/SKILL.md) | ABI manifest & lock, `syscall_abi.py --check` |
| **Linux, Android, POSIX shims** | [`bharat-os-personality`](file:///.agents/skills/bharat-os-personality/SKILL.md) | Compatibility shims strictly isolated from kernel core |
| **Verification, gates, smoke test** | [`bharat-os-validation`](file:///.agents/skills/bharat-os-validation/SKILL.md) | Five-target QEMU matrix, `verify.py`, evidence reporting |
| **Status, roadmap, gap analysis** | [`bharat-os-runtime-truth`](file:///.agents/skills/bharat-os-runtime-truth/SKILL.md) | `implementation_maturity.json` machine-readable ground truth |
| **Architecture docs, ADRs, contracts**| [`bharat-os-architecture-docs`](file:///.agents/skills/bharat-os-architecture-docs/SKILL.md) | ADR lifecycle, documentation hierarchy, CONTRACTS.md sync |
| **PR review, architectural audit** | [`bharat-os-review`](file:///.agents/skills/bharat-os-review/SKILL.md) | Pre-merge security, boundary, and diff-hygiene audit |
| **Board BSPs, manifests, linkers, MCUs** | [`bharat-os-embedded-bsp`](file:///.agents/skills/bharat-os-embedded-bsp/SKILL.md) | Board memory manifests, templated linkers, MMIO collision validation |
| **Nirmaan CLI, builds, tests, demos** | [`bharat-os-nirmaan`](file:///.agents/skills/bharat-os-nirmaan/SKILL.md) | Recommended developer CLI for builds, smoke tests, interactive runs, demos |

### Quick Context Pack Command
```bash
python3 tools/agents/context_pack.py --area <subsystem-name> --format compact
```

---

## 4. Definition of Done & Required Validation Gates

A task is ONLY complete when all relevant static linters, focused regression tests, and mandatory architecture gates are executed and observed with concrete evidence.

### Mandatory Five-Target Build & QEMU Matrix
All agents must verify changes against the five canonical hardware targets:

| Architecture | Canonical Target Profile YAML | Protection Model |
|---|---|---|
| **x86_64** | `delivery/targets/qemu/x86_64_desktop_headless.yaml` | MMU_FULL |
| **ARM64** | `delivery/targets/qemu/arm64_desktop_headless.yaml` | MMU_FULL |
| **RISC-V 64** | `delivery/targets/qemu/riscv64_desktop_headless.yaml` | MMU_FULL |
| **ARM32** | `delivery/targets/qemu/arm32_mmu_lite_headless.yaml` | MMU_LITE |
| **RISC-V 32** | `delivery/targets/qemu/riscv32_mmu_lite_headless.yaml` | MMU_LITE |

### Required Verification Commands
Agents are strongly encouraged to use `nirmaan` (`./nirmaan`, `.\nirmaan.ps1`, `nirmaan.bat`, or `python tools/nirmaan/cli.py`) for all building, testing, and target verification:

```bash
# 0. High-Productivity Agent & Developer Workflow (Recommended)
.\nirmaan.ps1 doctor                          # Environment doctor check
.\nirmaan.ps1 verify --all-boards             # Embedded board memory qualification
.\nirmaan.ps1 test board                      # Embedded board unit & qualification tests
.\nirmaan.ps1 build desktop-x86_64 --smoke    # Smoke build & test execution
.\nirmaan.ps1 build stm32f4-renode            # Renode embedded target build

# 1. Agent configuration and layer reference linters
python3 tools/agents/verify.py --changed-files <changed_files>

# 2. Syscall ABI compatibility (mandatory if syscalls/ABI touched)
python3 tools/abi/syscall_abi.py --check

# 3. Canonical 5-Target QEMU Smoke Matrix Gate
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```

### Required Completion Report Format
Every completed agent response must conclude with:
1. **Scope**: Exact list of files and subsystems modified.
2. **Architecture**: Invariants preserved or introduced (ownership, layer boundaries).
3. **Security**: Capability mediation, usercopy validation, and trust boundary impact.
4. **Verification Evidence**: Table reporting every gate with exact command and status (`PASS`, `FAIL`, `SKIPPED`, or `BLOCKED`).
5. **Documentation**: Contracts or docs updated (or explicit justification why not needed).
6. **Diff Hygiene**: Confirmation that zero generated files, third-party files, or unrelated modifications exist.
