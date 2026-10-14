---
name: bharat-os-syscall-abi
description: Apply when adding or changing syscalls, syscall metadata, UAPI arguments, capability requirements, usercopy phases, ABI lock, or generated dispatch artifacts.
---

# Bharat-OS Syscall ABI Skill

## Authority and Boundaries

- **Syscall Manifest**: `interface/contracts/abi/native_syscalls.json`
- **ABI Compatibility Lock**: `interface/contracts/abi/native_syscalls.lock.json`
- **ABI Validator & Generator**: `tools/abi/syscall_abi.py`
- Generated dispatch tables, headers, and stubs are **build artifacts only**; never hand-edit them in the source tree.

## Invariant Rules

1. **No Raw Syscall Numbers or Parallel Tables**:
   - Every system call must be formally specified in `native_syscalls.json`.
   - Never define handwritten dispatch tables or use raw magic integer syscall numbers in userland or kernel.
2. **Fixed-Width & Stable Data Types**:
   - Syscall arguments and structures crossing the UAPI boundary must use explicit fixed-width types (`uint32_t`, `uint64_t`, `int32_t`). Never use platform-dependent `long`, `unsigned long`, `size_t`, or raw pointer fields in UAPI structures.
3. **Capability Mediation**:
   - Every syscall must define its capability requirement in the manifest (capability type, required rights, scope).
   - Kernel dispatch automatically validates the capability before routing to the subsystem handler.
4. **Usercopy Discipline**:
   - Pointers passed in syscall registers are untrusted user addresses.
   - Kernel handlers must never dereference user pointers directly.
   - Pointers must be copied into kernel memory via `bh_usercopy_from_user()` with verified bounds before inspecting any fields (especially embedded capability handles).
5. **ABI Stability & Lock**:
   - Existing syscall numbers and argument orders are permanently frozen.
   - Changes to the manifest require running the ABI compatibility validator:
     ```bash
     python3 tools/abi/syscall_abi.py --check
     ```
   - The lock file is updated only when an intentional, reviewed, and approved ABI expansion occurs.

## Verification Checklist

- [ ] Run ABI check: `python3 tools/abi/syscall_abi.py --check`.
- [ ] Add positive test: successful invocation with valid arguments and capability.
- [ ] Add negative tests:
  - Invalid syscall number (must return `BH_STATUS_ERR_NOT_FOUND` or `BH_STATUS_ERR_INVALID_ARGUMENT`).
  - Missing capability right (must return `BH_STATUS_ERR_PERMISSION_DENIED`).
  - Stale generation / revoked capability (must return `BH_STATUS_ERR_STALE_HANDLE`).
  - Faulty user pointer / unmapped memory (must return `BH_STATUS_ERR_FAULT`).
