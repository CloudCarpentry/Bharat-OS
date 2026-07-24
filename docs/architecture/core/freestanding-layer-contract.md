---
title: "Freestanding Layer Contract (Kernel/HAL/Arch)"
status: "active"
owner: "Core Architecture Team"
version: "1.0"
last_updated: "2026-04-19"
tags: ["architecture", "layering", "freestanding", "lint"]
---

# Freestanding Layer Contract (Kernel/HAL/Arch)

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


This document formalizes a rule that already existed across multiple Bharat-OS architecture documents:

- **Kernel = mechanism**
- **Services = policy**
- **Lib = runtime glue**

The purpose of this contract is to stop architecture drift by making layer boundaries explicit and lint-enforced.

## 1) Freestanding boundary

| Layer | Freestanding | Allowed direct dependencies |
|---|---|---|
| `core/arch/` | ✅ Yes | `core/arch/` internals only, plus shared include/UAPI contracts |
| `corecore/hal/` | ✅ Yes | `corecore/hal/`, `core/arch/`, shared include/UAPI contracts |
| `core/kernel/` | ✅ Yes | `core/kernel/`, `corecore/hal/`, `core/arch/`, core/platform/boot glue, shared include/UAPI contracts |
| `lib/` | ❌ No (hosted) | `interface/uapi/`, runtime/services abstractions |
| `core/services/` | ❌ No (hosted) | `lib/`, `interface/uapi/` |
| `core/stacks/` | ❌ No (hosted) | `core/services/`, `lib/`, `interface/uapi/` |

> Design intent: kernel-side layers remain self-contained execution environments and do not absorb hosted policy/runtime concerns.

## 2) Hard rules

### Kernel-side (`core/kernel/`, `corecore/hal/`, `core/arch/`, `core/platform/`, `boot/`)

- Must remain freestanding in design and implementation.
- Must not depend on hosted user/runtime layers (`core/services/`, `core/stacks/`, top-level `lib/` contracts).
- Must not include hosted C runtime headers directly:
  - `<stdio.h>`
  - `<stdlib.h>`
  - `<string.h>`

### Hosted layers (`lib/`, `core/services/`, `core/stacks/`, `user/`)

- May use hosted/runtime constructs.
- Must consume kernel contracts via UAPI and defined interfaces, not kernel internals.

## 3) Enforcement

### Lint

`tools/lint/check_layer_references.py` is the contract linter for include-layer boundaries and freestanding header checks.

Examples:

```bash
python3 tools/lint/check_layer_references.py
python3 tools/lint/check_layer_references.py --strict --baseline tools/lint/baselines/layer_references.allowlist
python3 tools/lint/check_layer_references.py --report docs/reviews/gap_analysis/layer_reference_gap_analysis_YYYY-MM-DD.md
```

### CI

Kernel CI runs this linter in strict mode against the checked-in baseline to prevent newly introduced violations.

### Baseline policy

- `tools/lint/baselines/layer_references.allowlist` captures known debt.
- New violations are not permitted.
- Existing entries should be removed from the baseline as subsystems are refactored.

## 4) Migration note

Current repository state still contains historical placement/dependency debt (for example around storage/filesystem boundaries). This contract is the canonical rule for future movement: mechanism stays kernel-side; policy moves to services.
