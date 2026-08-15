# Actionable Remediation Plan

Based on the code quality and architecture review, here are the prioritized engineering tasks to close technical debt.

### Task 1: Enforce Strict Status Codes (P1)
- **Scope**: `core/services/` and `core/kernel/`
- **Action**: Deprecate `return -1;` for IPC, service endpoints, and internal handlers.
- **Criteria**: Use `bharat_status_t` (`BH_OK`, `BH_ERR_NO_MEMORY`, `BH_ERR_NOT_FOUND`, etc.). Refactor `namesvc`, `vm_manager`, `power_mode`, and `process_manager` tests/services.

### Task 2: CMake Linkage Hygiene (P1)
- **Scope**: All `CMakeLists.txt`
- **Action**: Lint and fix all `target_link_libraries` commands to explicitly use `PRIVATE`, `PUBLIC`, or `INTERFACE`.
- **Criteria**: Zero warnings when running CMake configuration for missing target scope.

### Task 3: Decouple Monolithic Subsystems (P2)
- **Scope**: `core/kernel/src/capability.c` & `core/kernel/src/sched/sched.c`
- **Action**:
  - Split `capability.c` into `cap_cspace.c` (cnode logic) and `cap_dispatch.c` (validation/invocation).
  - Split `sched.c` into runqueue management and scheduler API.
- **Criteria**: Max file size target ~800 lines. Better testability of individual capability/scheduling behaviors.

### Task 4: Safely Bound Payload Casting (P2)
- **Scope**: IPC Handlers (e.g. `accelmgr/main.c`, `devmgr/device_manager.c`)
- **Action**: Add explicit bounds-checking helpers before unsafe `(void*)` payload casting.
- **Criteria**: Reject malformed IPC messages cleanly using `BH_ERR_INVALID_ARGS` instead of triggering potential buffer overflows.

### Task 5: Centralize Standard Definitions (P3)
- **Scope**: `crypto_registry.c` and other C files redefining `NULL`.
- **Action**: Remove inline `#define NULL` and rely strictly on `<stddef.h>` or the internal `bharatlibc/include/standard/stddef.h`.
- **Criteria**: No rogue `#define NULL` found in `.c` sources.

### Task 6: Migrate Static State to Safe Allocation (P3)
- **Scope**: `core/services/vm_manager/vm_manager.c`
- **Action**: Remove static arrays like `g_vm_spaces[MAX_SPACES]` and implement boot-time pool allocations using the standard memory APIs.
- **Criteria**: Services should be safely restarteable without static state contamination.
