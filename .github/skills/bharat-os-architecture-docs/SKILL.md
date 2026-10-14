---
name: bharat-os-architecture-docs
description: Apply when writing or updating architectural documentation, Architecture Decision Records (ADRs), ROADMAP.md, README.md, BUILD.md, or contract specifications.
---

# Bharat-OS Architecture Documentation & ADR Skill

## Documentation Architecture

Bharat-OS enforces an evidence-backed documentation hierarchy. Documentation is not aspirational marketing; it is a binding technical contract.

```
docs/
├── architecture/           # Canonical subsystem architecture & layer models
│   └── CONTRACTS.md        # Central lookup registry of all system contracts & owners
├── adr/                    # Architecture Decision Records (ADR-001 through ADR-038+)
│   └── README.md           # Index of accepted, proposed, and deprecated ADRs
└── ai-agents/              # Cross-platform autonomous agent configuration guides

interface/contracts/        # Machine-readable authorities
├── abi/native_syscalls.json
└── implementation_maturity.json

Root Documentation:
├── AGENTS.md               # 4-Pillar Agent Constitution & Skill Routing Table
├── BUILD.md                # Single authoritative build, run, test, and debug guide
├── README.md               # Public repo overview & verified implementation status
└── ROADMAP.md              # Engineering milestones & verification backlog
```

## When to Author or Update an ADR

An ADR in [`docs/adr/`](file:///c:/workspace/Crazy_idea/Bharat-OS/docs/adr) is **mandatory** when a task:
1. Changes or introduces an object ownership model or cross-core synchronization protocol.
2. Changes or adds a public syscall, UAPI header, or capability rights bitmask.
3. Alters memory model dispatching or backend neutrality (`MMU_FULL`, `MMU_LITE`, `MPU/PMP`).
4. Introduces a new userland service, supervisor protocol, or bootstrap sequence.
5. Changes device profile policies (e.g. display tiers, automotive ECU safety constraints).
6. Alters the required validation matrix or QEMU execution gates.

### Canonical ADR Format

Save as `docs/adr/ADR-XXX-<descriptive-slug>.md`:
```markdown
---
title: <Short Descriptive Title>
status: Accepted | Proposed | Deprecated
date: YYYY-MM-DD
---

# Context
Describe the current state, underlying technical problem, limitations of existing shortcuts, and architectural motivators.

# Decision
State the exact technical decision:
- Clear layer placement (kernel, service, driver, arch, hal).
- Ownership and concurrency invariants.
- Capability mediation and fail-closed behaviors.
- Wire structures, bounded queues, and monotonic timeouts.

# Consequences
Document both positive and negative consequences:
- What is now guaranteed or proven.
- Trade-offs, memory or latency impact.
- What shortcuts are permanently prohibited.
```

## Guidelines for Updating Root Documents

### 1. `docs/architecture/CONTRACTS.md`
Whenever a contract, UAPI header, or service protocol changes:
- Update the owner subsystem, associated interface files, and required verification test suites.

### 2. `ROADMAP.md` & `README.md`
- **Strict Evidence Rule**: Never move a roadmap item to "Complete" or update a maturity table from "Partial" to "Implemented" without citing the exact executed test and QEMU console marker.
- Maintain consistency with `interface/contracts/implementation_maturity.json`.
- Keep the priority backlog prioritized: P0 (runtime blockers & safety), P1 (hardening & stress), P2 (scaffold replacement).

### 3. `BUILD.md`
- Keep `BUILD.md` in sync whenever:
  - New target profiles (e.g. GUI, automotive, mobile) are added to `delivery/targets/qemu/`.
  - New CMake presets or verification commands (`verify.py`, `run_qemu_matrix.py`) are introduced.
  - Cross-compiler toolchain requirements or emulator dependencies change.
