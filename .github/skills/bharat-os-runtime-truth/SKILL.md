---
name: bharat-os-runtime-truth
description: Apply when assessing status, completing roadmap milestones, updating maturity tables, or auditing repository health.
---

# Bharat-OS Runtime Truth Skill

## Core Principle

**Never rely on static roadmap documents, historical gap analyses, or aspirational comments as ground truth.**

A feature, subsystem, or service is ONLY considered implemented when verified against the three-part truth standard:
1. **Source Truth**: Implementation exists in the correct architectural layer.
2. **Build Truth**: Component is actively wired into `CMakeLists.txt` and compiles in release and debug configurations without synthetic bypasses.
3. **Execution Truth**: A real production authority adapter is installed (not returning `BH_STATUS_ERR_UNSUPPORTED` or dummy IDs), and the component emits its canonical runtime marker or passes automated QEMU smoke tests.

## Ground Truth Authority

The repository's authoritative, machine-readable inventory of implementation status is:
`interface/contracts/implementation_maturity.json`

Allowed maturity states:
- `PLANNED`: Architectural specification exists, code not yet written.
- `SCAFFOLD`: Types or stub functions present, but no production event loop or authority adapter.
- `PARTIAL`: Real logic implemented, but authority backend, fault handling, or E2E integration incomplete.
- `IMPLEMENTED`: Production authority adapter wired, functional tests pass, QEMU boots to ready marker.
- `HARDENED`: Fuzzing, race/concurrency tests, stress suites, and full 5-target matrix pass cleanly.

## Truth Audit Procedure

When asked "What is the status of subsystem X?", do NOT search for old markdown gap analyses. Run:
```bash
python3 tools/agents/context_pack.py --area <area-name> --format compact
```

Or run the maturity consistency checker:
```bash
python3 tools/check_implementation_maturity.py
```

## Updating Implementation Maturity

When completing a task that moves a subsystem forward:
1. Ensure unit/host tests and QEMU E2E tests pass.
2. Update `interface/contracts/implementation_maturity.json` with:
   - Updated `maturity` enum value.
   - Exact source file paths.
   - Specific, falsifiable `evidence` string citing executed tests and runtime markers.
3. Keep README and documentation tables synchronized with the JSON contract. Never manually update tables with claims not reflected in the JSON.
