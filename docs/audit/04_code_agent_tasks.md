# Code-Agent Tasks & Acceptance Criteria

This document translates the critical code quality and architectural issues into distinct, actionable tasks that a code-agent can execute autonomously.

## Task 1: Eliminate Generic Error Returns in IPC Handlers
**Context**: Multiple services use `return -1;` for IPC failures instead of defined `bharat_status_t` codes.
**Target Files**:
- `core/services/vm_manager/tests/test_vm_manager_caps.c`
- `core/services/namesvc/main.c`
- `core/services/process_manager/process_manager.c`
- `core/services/power_mode/state_machine.c`
- `core/services/legacy/net/control_plane.c`
**Instructions**:
1. Search for `return -1;` in the target files.
2. Replace `-1` with an appropriate `bharat_status_t` error code (e.g., `BH_ERR_NOT_FOUND`, `BH_ERR_NO_MEMORY`, `BH_ERR_UNAUTHORIZED`, or a service-specific error code if applicable).
**Acceptance Criteria**:
- No instances of `return -1;` remain in the specified files.
- The return types and signatures of the functions are respected.
- The project compiles successfully after changes.

## Task 2: Fix CMake Dependency Scope Leaks
**Context**: Several `CMakeLists.txt` use `target_link_libraries` without specifying scope (`PRIVATE`, `PUBLIC`, or `INTERFACE`), leading to global scope pollution.
**Target Files**:
- `core/services/common/runtime/CMakeLists.txt`
- `core/kernel/CMakeLists.txt`
- `core/lib/namesvc/CMakeLists.txt`
**Instructions**:
1. Locate `target_link_libraries` calls missing a scope keyword.
2. Add the `PRIVATE` keyword unless the linked library's headers are exposed in the target's public API (use `PUBLIC`), or if it's an interface-only library (use `INTERFACE`).
**Acceptance Criteria**:
- `grep -rn "target_link_libraries(" core/ | grep -v "PRIVATE" | grep -v "PUBLIC" | grep -v "INTERFACE"` returns 0 results.
- `tools/lint/check_cmake_dependencies.py` passes without new violations.
- Build completes successfully.

## Task 3: Refactor Unsafe Casts in Subsystem Managers
**Context**: IPC payload parsing uses raw `(void*)` casts directly on message buffers without bounds checking.
**Target Files**:
- `core/services/device/accelmgr/main.c`
- `core/services/device/devmgr/device_manager.c`
**Instructions**:
1. Identify raw casts of message payloads (`(void*)resp_buf`).
2. Wrap payload parsing with size validation checks to ensure the incoming message length matches the expected structure size before casting.
**Acceptance Criteria**:
- Payload unpacking functions validate `msg_length >= sizeof(ExpectedStruct)`.
- If invalid, the handler returns `BH_ERR_INVALID_ARGS` or equivalent before attempting a cast.

## Task 4: Remove Rogue NULL Redefinitions
**Context**: `NULL` is manually redefined in C files instead of using standard headers.
**Target Files**:
- `core/services/security/crypto/crypto_registry.c`
**Instructions**:
1. Remove `#define NULL ((void*)0)` from the target file.
2. Ensure `<stddef.h>` or `bharatlibc/include/standard/stddef.h` is included instead.
**Acceptance Criteria**:
- `grep -rn "#define NULL" core/` only shows results inside header files (like `stddef.h`).
- The project compiles without missing `NULL` definition errors.

## Task 5: Decouple Capability Registry (Oversized File Refactor)
**Context**: `core/kernel/src/capability.c` is nearly 1,400 lines long, making it hard to maintain.
**Target Files**:
- `core/kernel/src/capability.c`
**Instructions**:
1. Extract CSpace and registry-specific logic into a new file `core/kernel/src/cap_cspace.c`.
2. Extract capability invocation and dispatch logic into `core/kernel/src/cap_dispatch.c`.
3. Keep core generic initialization in `capability.c`.
4. Update `core/kernel/CMakeLists.txt` to include the new source files.
**Acceptance Criteria**:
- `core/kernel/src/capability.c` is under 800 lines.
- No functional regressions; the kernel boots and tests pass.

## Task 6: Audit and Remove Hardcoded Fallbacks
**Context**: Hardcoded IDs (like `g_system_device_id = 42`) mask configuration errors.
**Target Files**:
- `core/services/system/filesystem/main.c`
**Instructions**:
1. Remove the fallback assignment `g_system_device_id = 42`.
2. Implement proper error handling if the system device ID is not found or provided via boot configuration.
3. If not found, gracefully fail the filesystem initialization or enter a degraded state with appropriate logging.
**Acceptance Criteria**:
- `g_system_device_id` is initialized dynamically or via configuration, never hardcoded.
- Storage failures are reported rather than masked.
