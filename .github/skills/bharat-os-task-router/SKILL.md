---
name: bharat-os-task-router
description: Master routing skill for any non-trivial Bharat-OS task. Classifies task, queries current implementation maturity and context map, selects relevant domain skills, and establishes required reading, tests, and constraints.
---

# Bharat-OS Task Router Skill

Every agent working on Bharat-OS should start here before reading hundreds of repository files. This skill dynamically classifies the task, queries machine-readable repository truth, selects the minimal set of domain skills, and emits a focused execution manifest.

## Procedure

### Step 1: Classify Layer and Subsystem
Identify the architectural layer and primary subsystem from the request and modified files:
- `core/kernel/` -> **Kernel Mechanism** (scheduler, IPC, capabilities, PMM, VM primitives)
- `core/arch/` -> **Architecture Implementation** (x86_64, aarch64, riscv64, arm32, riscv32)
- `core/hal/` -> **Hardware Abstraction Layer** (architecture-neutral contracts, HAL glue)
- `platform/` -> **Platform Integration** (machine, board, SoC, device tree, memory layout)
- `core/drivers/` -> **Device Drivers** (serial, timer, interrupt controller, virtio, storage)
- `core/services/` -> **System Services** (vm_manager, process_manager, servicemgr, netmgr, devmgr)
- `core/stacks/` -> **Domain Protocol Stacks** (networking, storage, USB)
- `core/personalities/` -> **Compatibility Personalities** (Linux, Android, POSIX)
- `interface/` -> **Contracts & UAPI** (ABI manifests, headers, capability IDs)
- `delivery/targets/` -> **Targets & Profiles** (QEMU configs, board profiles)
- `tools/` -> **Tooling & Validation** (linters, generators, test runners)

### Step 2: Query Current Repository Truth
Never rely on stale roadmaps or memory. Query the machine-readable ground truth:
```bash
# Query compact context pack for the subsystem
python3 tools/agents/context_pack.py --area <area-name> --format compact

# Or query by changed files
python3 tools/agents/context_pack.py --changed-files <f1> <f2> --format compact
```
Inspect `interface/contracts/implementation_maturity.json` to verify current maturity (PLANNED, SCAFFOLD, PARTIAL, IMPLEMENTED, HARDENED).

### Step 3: Determine Memory Protection Model
Identify the memory protection backends affected by the change:
- `MMU_FULL`: Virtual memory with multi-level page tables, TLB shootdown, demand paging.
- `MMU_LITE`: Segmented/windowed protection without page tables.
- `MPU/PMP`: Region-based physical memory protection without address translation.
- `ALL`: Core abstractions that must fail closed on unsupported configurations.

### Step 4: Determine Concurrency & Ownership Model
Identify the concurrency model:
- `core-local`: State owned exclusively by one core (e.g. per-core runqueue, pcache magazine).
- `read-only shared`: Immutable configuration or hardware topology.
- `cross-core`: Coordination via bounded, by-value messages with monotonic deadlines (e.g. TLB shootdown).
- `distributed`: Multi-core transaction with request IDs, generations, rollback, and ACK/NACK.

### Step 5: Select ONLY Relevant Domain Skills
Load only the skills matching the classified area:
- Scheduler / IPC / Cross-core / uRPC -> `bharat-os-distributed-kernel`
- Kernel mechanisms / HAL / Driver placement -> `bharat-os-kernel-architecture`
- Memory / VM / PMM / MMU / MPU / DMA -> `bharat-os-memory-model`
- Capabilities / CSpace / Auth / Security -> `bharat-os-capability-security`
- Services / init / supervision / event loop -> `bharat-os-service-runtime`
- CMake / profiles / target configs -> `bharat-os-kernel-config`
- Syscalls / ABI / UAPI / Usercopy -> `bharat-os-syscall-abi`
- Linux / Android / POSIX personality -> `bharat-os-personality`
- Verification / smoke / five-target gates -> `bharat-os-validation`
- Status / milestone / maturity audit -> `bharat-os-runtime-truth`
- PR review / architectural audit -> `bharat-os-review`
- Board BSPs / memory manifests / linkers -> `bharat-os-embedded-bsp`
- Nirmaan CLI / builds / smoke / runs / demos -> `bharat-os-nirmaan`

### Step 6: Produce Execution Manifest
Before writing code, produce a concise manifest:
```yaml
area: <subsystem_name>
layer: <kernel|service|arch|hal|...>
maturity: <PLANNED|SCAFFOLD|PARTIAL|IMPLEMENTED|HARDENED>
skills:
  - <relevant-skill-1>
  - <relevant-skill-2>
files_to_read:
  - <primary source files>
contracts_to_read:
  - <relevant contract or architecture docs>
protection_models: [<MMU_FULL, MMU_LITE, MPU/PMP>]
concurrency_model: <core-local|cross-core|distributed>
tests_to_run:
  focused:
    - <subsystem specific unit/stress test>
  gates:
    - python3 tools/agents/verify.py --changed-files <...>
    - python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
forbidden_shortcuts:
  - No global mutable state
  - No pointer crossing across IPC/wire boundaries
  - No spin-count remote waits without monotonic deadlines
  - No hand-edited generated files
```
