# Syscall Production Hardening Review

## Status
The Bharat-OS syscall layer has been converted from a scaffold/stub level into a production-grade foundation.

## Changes
1. **Canonical UAPI Headers**: established `bh_syscall.h`, `bh_syscall_numbers.h`, and `bh_syscall_status.h`.
2. **ABI Manifest**: Added `syscall_manifest.json` and a CI-ready checker script to ensure append-only discipline.
3. **Metadata-Driven Dispatch**: Refactored the syscall table to use `bh_syscall_meta_t` for better auditability and security.
4. **Fail-Closed Dispatcher**: Hardened `bh_syscall_gate` to reject invalid/unsupported syscalls and enforce personality policies.
5. **Centralized Usercopy**: Implemented `bh_copy_from_user` and `bh_copy_to_user` to ensure no raw pointer dereferences in handlers.
6. **Mandatory Capability Validation**: All privileged syscalls now use centralized capability/rights validation.
7. **Removal of Fake Completeness**: Replaced silent stubs with explicit `BH_ERR_NOT_SUPPORTED`.
8. **SDK Cleanup**: ensured exactly one `bharat_syscall` symbol per architecture.

## Verification
- Host tests for status mapping and sysret convention passed.
- ABI integrity check passed.
- Grep audit confirms no active TODOs/stubs in the syscall dispatch path.
