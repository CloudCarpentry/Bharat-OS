# Bharat-OS Repository Agent Constitution

This file is the repository-wide source of truth for autonomous and interactive coding agents working on Bharat-OS, including Google Jules, OpenAI Codex, Google Antigravity, Gemini Code Assist/CLI, and GitHub Copilot.

The rules are mandatory unless a task explicitly changes an approved architecture contract. Tool-specific files may add narrower guidance, but they must not weaken this file.

## 1. Instruction precedence

Use this order when instructions conflict:

1. The explicit user/task requirement.
2. Security, safety, ABI, and architecture contracts in `interface/contracts/`, `docs/architecture/`, and accepted ADRs in `docs/adr/`.
3. The closest scoped `AGENTS.override.md`, `AGENTS.md`, Copilot path instruction, or Antigravity skill.
4. This root `AGENTS.md`.
5. General repository documentation and existing local conventions.

Do not guess when two authoritative contracts conflict. Record the conflict, preserve the safer existing behavior, and mark the task blocked unless the task explicitly resolves it.

## 2. Mission and architecture evolution

Bharat-OS is a verification-oriented, capability-based microkernel evolving through three compatible phases:

1. **Core microkernel:** a small trusted computing base containing mechanisms only.
2. **Per-core kernel:** core-local ownership, local scheduling, and explicit cross-core coordination.
3. **Distributed kernel:** message-based distributed object lifecycle and ownership across cores and, later, nodes.

Code may be transitional, but new work must move toward the target architecture and must not re-centralize state merely because a centralized shortcut is easier.

### Non-negotiable architectural principles

- Keep the kernel minimal and verification-friendly.
- Kernel owns mechanisms; services own policy and orchestration.
- Every resource access is capability-mediated.
- Deny by default and fail closed.
- No shared mutable cross-core state without an explicit, documented ownership protocol.
- Cross-core actions use bounded message-based protocols, not remote direct mutation.
- Support MMU, MMU-Lite, and MPU configurations through backend-neutral contracts.
- Use HAL/runtime discovery for dynamic hardware facts; do not hardcode them in kernel configuration.
- Maintain strict `arch/`, `hal/`, and `platform/` separation.
- Build/runtime/documentation claims must match actual evidence.

## 3. Layer and folder boundaries

Before adding a file, identify its correct layer.

- `core/kernel/`: scheduling, memory mechanisms, syscall/trap mechanisms, capabilities, IPC/uRPC primitives, faults, and minimal coordination only.
- `core/arch/`: ISA/CPU-specific implementation.
- `core/hal/`: architecture-neutral contracts and common glue.
- `platform/` or the repository's current platform zone: board, machine, SoC, topology, and interconnect wiring.
- `core/drivers/`: hardware control and driver mechanisms.
- `core/services/`: policy, managers, lifecycle, routing, and orchestration.
- `core/stacks/`: composed protocol/domain stacks.
- `core/lib/`: reusable implementation logic that is not kernel policy.
- `interface/`: stable external contracts, SDK/UAPI, IDL, and ABI authorities.
- `quality/`: tests, stress suites, benchmarks, and verification evidence.
- `delivery/`: targets, profiles, packaging, release and execution descriptions.
- `tools/`: generators, linters, build orchestration, and validators.

Do not move policy into `core/kernel/`, ISA implementation into `core/hal/`, or real drivers into services.

## 4. Kernel ownership and distributed-operation rules

For scheduler, VM, PMM, capabilities, IPC/uRPC, monitor, and other cross-core work:

- Identify the owner core for every mutable object.
- Prefer core-local registries, queues, transaction tables, caches, and counters.
- Never pass raw pointers, stack addresses, allocator addresses, function pointers, or architecture-sized `long` fields across IPC/uRPC/wire boundaries.
- Use fixed-width, versioned, by-value wire structures.
- Add compile-time size and offset assertions for wire-visible structures.
- Use explicit request IDs and generations to prevent stale-handle/ABA confusion.
- Use bounded queues and explicit backpressure or `BUSY`/retry behavior.
- Use monotonic HAL time for deadlines. Do not use spin-count timing.
- Bound retries and retry only missing/unacknowledged participants.
- Make receivers idempotent and replay-safe when requests may be retried.
- Do not hold an address-space, scheduler, capability-table, or object lock while waiting for remote completion.
- Publish local state only after required remote acknowledgement.
- Define rollback/compensation for partial failure.
- If safe rollback is impossible, poison or quarantine the affected object and fail closed.
- Destruction must have an explicit lifecycle state such as `DYING`; reject new mutations after destruction begins.

Every new mutable kernel structure must document whether it is:

- core-local,
- immutable/read-only shared,
- lock-protected shared as a temporary migration exception, or
- distributed with an explicit ownership and message protocol.

## 5. Capability and syscall trust boundary

`interface/contracts/abi/native_syscalls.json` is the authority for the native syscall boundary.

- Never bypass the contract with raw syscall numbers or parallel hand-written tables.
- Do not hand-edit generated syscall headers/tables.
- Validate capability object type, rights, scope, generation/liveness, and ownership as required by the contract.
- Capability-bearing data embedded in user structures must be validated only after fault-safe usercopy.
- Do not dereference user pointers before the approved usercopy boundary.
- Preserve stable ABI numbers and semantics unless an explicitly approved ABI migration says otherwise.
- Update the lock only when the task intentionally changes the contract and the compatibility review is complete.

Required ABI check after syscall-related work:

```bash
python3 tools/abi/syscall_abi.py --check
```

## 6. Kernel configuration system

The kernel configuration is generated from a template.

- Source template: `core/kernel/include/bharat_config.h.in`
- Generated header: `build/<target>/generated/include/bharat_config.h`

Rules:

- Never commit generated `bharat_config.h` files.
- Modify `core/kernel/include/bharat_config.h.in` for new compile-time configuration.
- Use CMake `@VAR@` or `#cmakedefine` syntax as appropriate.
- Keep all generated artifacts under `${CMAKE_BINARY_DIR}`.
- Include the configuration as `#include "bharat_config.h"`.
- Keep the fail-fast source-tree shim unchanged unless the task specifically changes shim behavior.
- Configuration must be deterministic.
- Dynamic hardware capabilities must be discovered through HAL/platform mechanisms.
- A configuration change must consider MMU, MMU-Lite, and MPU profiles. Unsupported combinations return an explicit unsupported status and fail closed.

## 7. Naming and C coding conventions

Apply these rules to new kernel code and to public APIs touched by a task. Do not perform unrelated mass renames.

### Bharat-OS naming

- Public kernel functions: `bh_<subsystem>_<operation>()`.
- Public types: `bh_<name>_t`.
- Public struct/enum tags: `bh_<name>` where tags are exposed.
- Constants, macros, enum values, rights, status values, and compile-time feature names: `BH_<SUBSYSTEM>_<NAME>`.
- New public kernel headers: prefer `bh_<subsystem>.h` or an existing canonical subsystem header.
- Internal file-local helpers: descriptive `snake_case`; prefix with the subsystem when ambiguity is likely.
- Existing stable ABI names are not renamed without an approved migration.

### C and low-level safety

- Follow the repository's established C style and `.clang-format`; do not invent a competing style.
- Use fixed-width integer types for ABI/wire/hardware layouts.
- Check integer overflow, truncation, alignment, range, and multiplication before allocation or copy.
- Avoid undefined behavior, unaligned access assumptions, and strict-aliasing violations.
- Do not use magic numbers for register offsets, rights, wire fields, page flags, or protocol states.
- Check all fallible return values.
- Use the repository status type and canonical error codes; do not invent ad hoc negative integers.
- Initialize structures explicitly, including reserved fields that cross trust boundaries.
- Keep critical sections bounded. Never sleep or wait for remote completion while holding a spinlock.
- Add comments for invariants, ownership, memory ordering, lock ordering, and non-obvious safety—not for obvious syntax.
- TODOs must name a concrete missing behavior or issue; never leave a vague “fix later.”

### Concurrency and atomics

- State the synchronization and ownership model in code comments near the data definition.
- Use the weakest correct atomic ordering only when justified; otherwise follow the established subsystem pattern.
- Do not mix atomic and non-atomic access to the same mutable field.
- Add race, replay, timeout, and saturation tests for concurrent protocols.

## 8. Mandatory task workflow

### Before editing

1. Read the root `AGENTS.md` and any closer scoped instructions.
2. Read the relevant architecture contract and ADRs.
3. Inspect the existing implementation and tests before proposing a design.
4. Identify affected architectures, memory-protection backends, capabilities, generated artifacts, and documentation.
5. Write a concise plan with invariants, failure behavior, tests, and documentation impact.

### While editing

- Make the smallest coherent change that closes the task.
- Preserve unrelated behavior and avoid opportunistic refactors.
- Add or update tests in the same change.
- Keep generated files out of source control unless the repository explicitly tracks that generated artifact.
- Update architecture/contract documentation when behavior, ownership, lifecycle, ABI, configuration, build, or validation changes.

### Before completion

- Inspect the final diff for scope creep, generated files, pointer-crossing, direct remote mutation, raw syscall numbers, and stale docs.
- Run applicable focused tests first, then the mandatory repository gates.
- Report every command with PASS, FAIL, SKIPPED, or BLOCKED.
- Never claim a test passed unless it was actually executed and evidence was observed.
- A missing toolchain, target, YAML, emulator, flag, or test is `BLOCKED`, not `PASS`.

## 9. Required validation gates

### Static and contract checks

Run applicable repository linters and contract checks:

```bash
python3 tools/lint/check_layer_references.py
python3 tools/lint/check_cmake_dependencies.py
python3 tools/abi/syscall_abi.py --check
```

The ABI command is mandatory for syscall/ABI changes and optional otherwise.

### Mandatory five-target build policy

All agents must run and verify all five target commands prior to declaring a task complete or submitting a PR:

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
```

Do not silently replace a required target with a different target. If a required target is not implemented in the current branch, record the gate as `BLOCKED: target unavailable` and do not claim full-matrix completion.

### Mandatory QEMU matrix policy

Preferred command:

```bash
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```

Before running, inspect `--help`. If the current runner does not support `--all-arch`, run the currently supported partial command for additional evidence:

```bash
python3 tools/run_qemu_matrix.py --headless --smoke
```

Then report the mandatory gate as blocked until `--all-arch` and every required target are supported. A runner that silently skips missing target YAML files does not constitute full-matrix success.

### Focused test expectations

- Bug fixes require a regression test that fails before the fix and passes after it.
- Cross-core changes require SMP, timeout, retry, replay, queue-full, stale-generation, and partial-failure coverage where applicable.
- Memory changes require leak, double-free, map/protect/unmap lifecycle, rollback, and backend coverage.
- Capability changes require positive and negative rights/type/scope/stale/revoked tests.
- ABI changes require manifest, lock, generator, generated-output, and raw-number checks.
- Architecture changes require at least one focused target build/run for each affected ISA before the full matrix.

## 10. Documentation obligations

Update documentation in the same change when implementation truth changes.

Consult and update as applicable:

- `interface/contracts/` — machine-readable authority.
- `docs/architecture/CONTRACTS.md` — human lookup map for contracts and owners.
- `docs/architecture/` — architecture and invariants.
- `docs/adr/` — decisions, rationale, alternatives, and migration consequences.
- `BUILD.md` — build, target, emulator, and validation commands.
- `CONTRIBUTING.md` — coding and contribution workflow.
- `delivery/targets/` — target truth.
- README maturity/status tables — only when backed by evidence.

Create or update an ADR when a change introduces or alters:

- ownership or cross-core protocol,
- public ABI/UAPI,
- capability lifecycle or trust boundary,
- memory authority path or backend semantics,
- kernel/service/driver placement,
- persistent format,
- compatibility policy,
- required build or release gate.

## 11. Do-not-touch and prohibited shortcuts

- Do not modify `ext/` or `external/` unless the task explicitly requires a reviewed third-party update.
- Do not commit build directories, generated configuration headers, local logs, emulator dumps, caches, or toolchain artifacts.
- Do not bypass `native_syscalls.json` or capability validation.
- Do not introduce direct architecture-backend calls where a domain/HAL operations table is authoritative.
- Do not add kernel policy that belongs in a service.
- Do not use global mutable state to avoid designing ownership.
- Do not disable tests, weaken assertions, increase timeouts, or suppress warnings merely to obtain a green result.
- Do not claim unsupported components are complete in docs or maturity tables.
- Do not change unrelated code, formatting, or generated output.
- Do not commit, push, merge, or open a PR unless the task explicitly requests it.

## 12. Required completion report

Every completed agent task must report:

1. **Scope:** files/subsystems changed.
2. **Architecture:** invariants preserved or introduced.
3. **Security:** capability/trust-boundary impact.
4. **Tests:** exact commands and PASS/FAIL/SKIPPED/BLOCKED status.
5. **Matrix:** status of all five required targets and the QEMU all-architecture gate.
6. **Documentation:** files updated or reason no update was required.
7. **Risks:** known limitations, follow-up issues, or unverified assumptions.
8. **Diff hygiene:** confirmation that generated/third-party/unrelated files were not included.

A task is not complete when required validation is failed or blocked. It may be delivered as a transparent partial result, but must not be represented as done.
