# Syscall / Kernel / SDK / Lib Conflict Analysis (2026-04-26)

## Scope

This review covered:
- Kernel build and syscall integration paths
- SDK and userspace syscall stubs
- Headless cross-arch builds requested (`x86_64_desktop_headless`, `arm64_desktop_headless`, `riscv64_desktop_headless`)
- User library and base test app compile status (`bharat_syscall`, `bharatsys`, `bharatc`, `app_test`)

## What was fixed during this pass

1. **HAL page-size macro/enum name collision (hard compiler break).**
   - `core/hal/include/hal/hal_mmu.h` defined both preprocessor macros and enum members with the same names (`HAL_PAGE_SIZE_4K`, etc.), causing compile errors.
   - Resolved by keeping the macro constants as the single source in that header.

2. **x86_64 HAL duplicate symbol definition (hard compiler break).**
   - `core/arch/hal/x86/x86_64/hal_pt_x86_64.c` had duplicate file-scope definitions of `g_x86_pcid_supported`.
   - Removed duplicate to restore one-definition correctness.

3. **Runtime include-path mismatch between old/new UAPI header layouts.**
   - `core/lib/runtime` targets were missing include paths needed by legacy `<uapi/...>` includes used by runtime SDK sources.
   - Added `${CMAKE_SOURCE_DIR}/interface` and `${CMAKE_SOURCE_DIR}/interface/include` to runtime target include paths.

4. **Cross-arch HAL/MM linkage coverage (x86_64/arm64/riscv64).**
   - Added missing `hal_mm_get_zone_limits()` and `hal_mm_backend_caps()` in x86_64 and arm64 HAL MM backends.
   - Added missing `hal_mem_get_caps()` implementation and concrete `hal_get_arch_capabilities()` return path in riscv64 HAL MM backend.

## Build and validation outcome

### QEMU toolchains
QEMU packages were installed and verified:
- `qemu-system-x86_64`
- `qemu-system-aarch64`
- `qemu-system-riscv64`

### Requested headless builds
- `./build.sh all --target x86_64_desktop_headless` -> **PASS** (build + package + QEMU boot marker observed).
- `./build.sh all --target arm64_desktop_headless` -> **PASS** (build + package + QEMU boot marker observed).
- `./build.sh all --target riscv64_desktop_headless` -> **FAIL** (build + package succeed, QEMU run times out before boot marker).

Status: link-time HAL/MM symbol blockers are fixed for x86_64/arm64/riscv64. Remaining execution blocker is RISC-V runtime boot progress (OpenSBI handoff occurs, kernel boot marker not emitted within runner timeout).

### User lib/base test app compile status
For all configured presets, these targets build successfully:
- `bharat_syscall`
- `bharatsys`
- `bharatc`
- `app_test`

Command used:
- `cmake --build --preset=<preset> --target bharat_syscall bharatsys bharatc app_test`

## High-risk syscall contract finding (important)

There are currently **two syscall number header families** active in-tree:

1. `interface/include/bharat/uapi/syscall_nr.h`
   - Canonical table driven by `syscall_table.def` (newer ABI surface).

2. `interface/uapi/syscall/syscall_nr.h`
   - Legacy macro table (contains calls like `SYSCALL_MEM_ALLOC`, `SYSCALL_THREAD_EXIT`, `SYSCALL_IPC_CALL`, etc.).

Current runtime and SDK wrapper code still references legacy names in several places. This can create silent ABI drift depending on include paths and target configuration.

## Recommended next steps to fully stabilize syscall work

1. **Declare a single canonical syscall source-of-truth** (prefer `interface/include/bharat/uapi/syscall_table.def`).
2. **Add explicit compatibility alias policy** (temporary aliases in one place) or migrate all callsites in one sweep.
3. **Add a CI check** that rejects dual-source syscall definitions and unknown syscall identifiers in SDK/runtime wrappers.
4. **Fix kernel HAL/MM link breakage** (`hal_mm_backend_caps`, `hal_mm_get_zone_limits`, etc.) so QEMU boot verification can run.
5. **After link fixes**, run headless QEMU smoke tests per arch with syscall-focused app/test payloads.

