# Bharat-OS Syscall ABI Contract

Version: 1.0
Status: Frozen

## 1. Overview
The Bharat-OS Syscall ABI defines the stable interface between userspace applications (via the SDK) and the kernel.

## 2. Canonical Headers
- `interface/include/bharat/uapi/syscall/bh_syscall_numbers.h`: Canonical source for syscall numbers.
- `interface/include/bharat/uapi/syscall/bh_syscall_status.h`: Canonical source for native status codes (`bh_status_t`).
- `interface/include/bharat/uapi/syscall/bh_syscall.h`: Main entry point for userspace.

## 3. ABI Rules
- **Append-only**: Syscall numbers are strictly append-only.
- **No Renumbering**: Never renumber, reorder, or reuse existing syscall numbers.
- **Uniformity**: No architecture-specific syscall numbers.
- **Status Codes**: Native syscalls return `bh_status_t`. Errors are negative, success is `BH_OK (0)` or positive.

## 4. Manifest
The ABI is guarded by `contracts/abi/syscall_manifest.json` and verified by `tools/abi/check_syscall_abi.py` in CI.
