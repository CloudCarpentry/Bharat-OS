# Plan: Cross-Host Build/Test/Run Validation (Linux + Windows)

This plan defines a structured rollout to validate Bharat-OS build and execution workflows on Linux and Windows hosts.

---

## Objectives

- Standardize host workflows for `riscv64`, `arm64`, and `x86_64`
- Ensure reproducible build/test/run steps across Linux and Windows
- Introduce simulator depth in phases (QEMU first, then specialized tools)

---

## Phase 1 — Baseline Host Readiness

**Deliverables**
- Confirm developer machines and CI runners satisfy `docs/ENV_PREP.md`
- Confirm CMake presets and build scripts run on both hosts

**Acceptance criteria**
- `cmake --list-presets` works on Linux and Windows
- `tools/build.sh` and `tools/build.ps1` execute successfully

---

## Phase 2 — Build Matrix Validation

**Deliverables**
- Build kernels on both hosts for:
  - `riscv64`
  - `arm64`
  - `x86_64`

**Acceptance criteria**
- All selected architecture builds succeed on both hosts
- Artifacts are generated in expected build folders

---

## Phase 3 — Host Test Gate

**Deliverables**
- Enable host-side test execution in Linux + Windows pipelines
- Publish test summaries in CI artifacts

**Acceptance criteria**
- `ctest --preset run-tests` passes on both hosts
- Failing tests block merge by policy

---

## Phase 4 — QEMU Runtime Smoke Gate

**Deliverables**
- Add QEMU smoke boot jobs for architecture tiers (at least `riscv64` and `x86_64`; `arm64` as available)
- Store serial logs as CI artifacts

**Acceptance criteria**
- Kernel reaches expected early boot markers in serial output
- Runtime regressions are detectable from logs

---

## Phase 5 — Advanced Simulator Tracks (Profile-Based)

**Deliverables**
- RISC-V ISA validation track (Spike)
- Embedded/peripheral track (Renode)
- Micro-architecture/perf track (gem5)
- x86 boot deep-debug track (Bochs)

**Acceptance criteria**
- Each subsystem/profile has one designated deep-validation simulator
- Nightly jobs provide pass/fail trend visibility

---

## Execution Backlog

1. Add CI jobs for Linux + Windows host tests
2. Add build matrix jobs (`riscv64`, `arm64`, `x86_64`)
3. Add QEMU smoke boot with serial log capture
4. Add Spike/Renode/gem5/Bochs optional nightly jobs
5. Add release checklist tying artifact promotion to gate results

---

## Risks and Mitigations

- **Toolchain drift across hosts**  
  Mitigation: pin versions in CI images and setup docs.

- **QEMU machine/cpu mismatch by arch**  
  Mitigation: maintain tested command templates per architecture.

- **Long-running advanced simulations**  
  Mitigation: run deep simulations nightly, not per-PR.

---

## Exit Criteria (Plan Complete)

The plan is considered complete when:

- Linux + Windows host tests are mandatory and stable
- Multi-arch build matrix is mandatory and stable
- QEMU smoke runtime gates kernel changes
- At least one advanced simulator track is operational for each target profile family

