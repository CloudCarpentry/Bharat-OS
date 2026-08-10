---
title: Userspace Runtime Models and Root Bootstrap
status: Draft
owner: Userspace Architecture Team
---

# Userspace runtime models

Bharat-OS resolves four independent target dimensions into one bootstrap choice:

1. hardware and device profile;
2. execution profile;
3. memory-protection model; and
4. `userspace.runtime_model`.

The first three dimensions prove mechanisms and constraints. The fourth selects
userspace lifecycle policy. It is never inferred from architecture, device class,
scheduler profile, MMU/MMU-Lite/MPU selection, personality, or discovered hardware.

| Model | Authoritative root | Lifecycle owner | Direct acceptance evidence |
|---|---|---|---|
| `direct` | `experience/user/apps/user_smoke` | application | `USER_SMOKE_ENTERED`, `STARTUP_ABI_OK`, `SYSCALL_OK`, `USER_SMOKE_COMPLETE` |
| `static` | `core/services/core/rt-supervisor` | deterministic static supervisor | `RT_SUPERVISOR: ENTERED`, `RT_RUNTIME: STABLE` |
| `light` | `core/services/core/init-lite` | bounded lightweight supervisor | `INIT_LITE: ENTERED`, `LIGHT_RUNTIME: STABLE` |
| `full` | existing `core/services/core/init` | normal managed service environment | existing init/handoff/service-graph evidence |

## Configuration and packaging flow

```text
target YAML
  -> schema validation
  -> target resolver (missing value explicitly defaults to full)
  -> BHARAT_USERSPACE_RUNTIME_MODEL and numeric generated configuration
  -> one root component
  -> existing boot-module container
  -> architecture-neutral module discovery and root loader
  -> generic startup ABI plus root-launch extension
```

Canonical target syntax is:

```yaml
userspace:
  runtime_model: direct
```

There is no top-level alias and no per-architecture default. `full` is only a
migration default for targets that omit the nested field.

## ABI boundaries

- `bharat_user_startup_t` remains generic process/thread startup state.
- `bharat_bootstrap_info_t` remains boot instance and initial endpoint state.
- `bh_root_launch_info_t` is immutable root-only lifecycle selection metadata.

The extension flag and append-only placement preserve the existing startup
layout. Consumers must check the flag, version, and size before reading it.
The launch structure is not a capability and grants no authority.

## Qualification boundaries

DIRECT is the first runtime gate because it has no manager or graph dependency.
The five required architectures must still be built and smoked independently.
Existing architecture-entry failures are reported with the last successful and
expected next marker; this model does not turn those failures into success.
