# Bharat-OS: Critical Code Quality & Architecture Audit

This report highlights code-level defects, structural risks, and technical debt found across the repository, translating them into actionable tasks.

## 1. Oversized & Monolithic Files
Several core kernel and service components have grown extremely large (1,000+ lines), making them hard to maintain and violating modularity principles.

*   **`core/kernel/src/capability.c` (1380 lines)**: Centralized capability handling needs splitting into `cap_dispatch.c`, `cap_cspace.c`, `cap_revoke.c`.
*   **`core/kernel/src/sched/sched.c` (1331 lines)** and **`core/kernel/src/sched/sched_core.c` (1155 lines)**: Scheduler monolithic. Needs decoupling of scheduling queues vs scheduling policies.
*   **`core/kernel/src/mm/pmm/pmm.c` (1235 lines)**: Physical memory manager is too large. Split initialization, buddy allocation, and page fault handling.
*   **`core/arch/.../hal_pt_*.c` (600-900 lines)**: Page table manipulation logic is highly duplicated across architectures.

## 2. Weak Error Handling
Multiple services and kernel subsystems use generic `-1` error returns instead of defined, expressive status codes (e.g., `BH_ERR_NOT_FOUND`, `BH_ERR_NO_MEMORY`).

*   **IPC Message Receivers**: `test_vm_manager_caps.c`, `test_process_manager_caps.c`, `namesvc/main.c` return `-1` silently on failure.
*   **State Machines**: `core/services/power_mode/state_machine.c` returns `-1` continuously.
*   **Legacy Network**: `core/services/legacy/net/control_plane.c` and `data_plane.c` use `-1` instead of network-specific error codes.
*   **Actionable fix**: Replace generic `-1` returns with `bharat_status_t` enums across IPC bounds and core internal APIs.

## 3. Hidden Global State
Significant usage of file-local `static` state (over 2,100 instances), especially large arrays, which prevents multi-tenancy and complicates safe tear-down/restart of services.

*   **VM Manager**: `core/services/vm_manager/vm_manager.c` relies on massive static arrays (`g_vm_spaces`, `g_vm_regions`, `region_table`). This needs dynamic slab allocation or bounded pool initialization at boot to scale.
*   **Device & Auth State**: Similar static structures found across multiple drivers and unprivileged services.

## 4. CMake Dependency Leaks
Target linkage relies on `INTERFACE` without encapsulating private dependencies, risking accidental pollution of the global build environment.

*   `core/services/common/runtime/CMakeLists.txt:17` uses naked `target_link_libraries` without scope (`PUBLIC`/`PRIVATE`).
*   `core/kernel/CMakeLists.txt:693` dumps over 10 subsystems into `kernel.elf` without scoping.
*   **Actionable fix**: Audit all `CMakeLists.txt` and enforce `PUBLIC`, `PRIVATE`, or `INTERFACE` modifiers.

## 5. Unsafe Casts & Macro Redefinitions
*   **`NULL` redefinition**: Found `#define NULL ((void*)0)` duplicated in `crypto_registry.c` and standard headers, risking conflicts with system `stddef.h`.
*   **Raw `(void*)` Casts**: Over 68 raw memory casts observed, especially in `accelmgr/main.c` and `devmgr/device_manager.c` when casting message payloads to response buffers. These need structured `memcpy` bounds checks or zero-copy validation prior to casting.

## 6. Duplicated Fallbacks & "TODO" Debt
*   Over 120 "fallback" workarounds in the codebase (e.g. software queue fallbacks, default IDs like `42` in `system/filesystem/main.c`).
*   90+ unresolved `TODO` markers. Crucial missing features like IPC blocking receive (`netmgr/src/main.c`), dynamic vaddr allocation (`aspace.c`), and capability handle rebinding (`service_runtime.c`).
