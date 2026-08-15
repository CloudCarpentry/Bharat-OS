# File-Level Defect Audit

This report highlights specific file-level issues identified during the code-level architecture audit.

## 1. Unsafe Casts & Alignment Issues
- **`core/services/device/accelmgr/main.c`**: Payload payloads coming from `bharat_ipc_recv` were blindly cast to structures without prior bounds checking to ensure `msg->header.payload_size` met the required size. This has been remediated.
- **`core/services/device/devmgr/device_manager.c`**: While device mmio handling casts `out_window` without prior type checking from lookup implementations, this isn't exposed via IPC. Still, proper strict aliasing discipline is needed.

## 2. Weak Error Handling
Multiple core services silently returned `-1` when they encountered IPC or internal errors, dropping valuable debugging information. The following have been remediated:
- `core/services/vm_manager/tests/test_vm_manager_caps.c` (Now `BHARAT_IPC_STATUS_ERR_NOT_FOUND`)
- `core/services/process_manager/process_manager.c` (Now `BHARAT_STATUS_ERR_INTERNAL` and `BHARAT_STATUS_ERR_NOT_FOUND`)
- `core/services/power_mode/state_machine.c` (Now `BHARAT_STATUS_ERR_INTERNAL` and `BHARAT_STATUS_ERR_NOT_FOUND`)
- `core/services/legacy/net/control_plane.c` (Now `BHARAT_STATUS_ERR_INTERNAL` and `BHARAT_STATUS_ERR_NOT_FOUND`)

*Note: `core/services/namesvc/main.c` returned an IPC enum from `main()`, but since `main()` expects standard exit codes like `-1`, this file was restored.*

## 3. Hardcoded Fallbacks
- **`core/services/system/filesystem/main.c`**: Hardcoded `g_system_device_id = 42` was being utilized as a silent fallback when the system boot device lookup failed. This disguised boot errors and has been remediated to fail early with `-1`.

## 4. CMake Dependency Leaks
- **`core/services/common/runtime/CMakeLists.txt`** and **`core/lib/namesvc/CMakeLists.txt`**: Both used naked/public `target_link_libraries` that polluted the global target namespace. Fixed to explicitly use `PRIVATE` dependencies.
