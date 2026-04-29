# Bharat-OS Syscall ABI Contract

This document defines the ABI stability rules for Bharat-OS syscalls.

## Rule 1: Append-Only

The syscall table is append-only. Syscall numbers, once assigned to a `stable` syscall, must never be changed, reordered, or reused for a different purpose.

## Rule 2: Manifest-Driven Validation

The `tools/abi/check_syscall_abi.py` tool enforces ABI stability by comparing the UAPI header (`interface/include/bharat/uapi/syscall/bh_syscall_numbers.h`) against the stable manifest (`contracts/abi/syscall_manifest.json`).

## Syscall Statuses

The manifest tracks the status of each syscall:

*   **stable**: Frozen ABI. Must exist in the header with the same name and number.
*   **reserved**: The number is reserved and must not be used in the header.
*   **deprecated**: The syscall is supported but scheduled for removal. The number cannot be reused.
*   **experimental**: Initial implementation. May change before being promoted to `stable`. Should typically be guarded by build flags.

## Status Code Stability

Bharat-OS uses a stable set of status codes (`bh_status_t`) defined in `interface/include/bharat/uapi/syscall/bh_syscall_status.h`. These codes are translated from internal `kstatus_t` and must remain numerically stable across releases.

## Metadata Coverage

Every `stable` syscall must have a corresponding entry in the kernel's metadata table (`bh_syscall_meta_t`). This metadata drives the syscall gate's policy enforcement, including argument count validation, capability checks, and audit logging.

## ABI Checker Requirements

1.  **Renumbering Detection**: Detect if a manifest syscall has a different number in the header.
2.  **Removal Detection**: Detect if a `stable` or `deprecated` syscall is missing from the header.
3.  **Count Validation**: Ensure `BH_SYSCALL_COUNT` correctly reflects the number of syscalls (including holes).
4.  **Reserved Number Guard**: Ensure `reserved` numbers are not accidentally used.
5.  **New Syscall Placement**: Ensure new syscalls are only added at the end of the table.
