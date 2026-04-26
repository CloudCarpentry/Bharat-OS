# Init Service

## Responsibility
Temporary bootstrap coordinator.

## Non-responsibility
- Not a permanent service supervisor.
- Does not own long-running policy after handoff.

## Boot context
Uses `init_boot_context_t` from `interface/include/bharat/uapi/init/init_boot_context.h`.
Validation is performed via `init_boot_context_is_valid`.

## Profile model
Selected runtime profile is an enum.
Manifest service applicability uses profile masks.

## Manifest validation
Dependency validation runs before `CORE_STARTING` using `init_graph_validate`.
It checks for:
- Duplicate service IDs
- Unknown or filtered dependencies
- Dependency cycles
- Absence of CORE services
- Missing required capabilities for critical services

## Bootstrap-deferred mode
Kernel may report that the userspace loader is not wired.
This is explicit degraded/bootstrap-deferred behavior, not a successful user init launch.
It is indicated by the outcome `INIT_BOOT_OUTCOME_BOOTSTRAP_DEFERRED`.
