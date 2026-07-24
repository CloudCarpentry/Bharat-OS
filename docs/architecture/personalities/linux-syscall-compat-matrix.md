# Linux Syscall Compatibility Matrix

## Status
Accepted

## Scope
Mapping between Linux syscalls and Bharat-OS.

## Contract
- Linux personality uses `normalize_syscall_return` to map `kstatus_t` to `-errno`.
- Unsupported syscalls return `-ENOSYS`.

## Failure Behavior
Negative `kstatus_t` in range [-1, -1000] are translated via `linux_errno_from_bh_status`.
