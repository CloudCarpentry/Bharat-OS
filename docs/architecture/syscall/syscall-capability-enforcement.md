# Syscall Capability Enforcement

## Status
Accepted

## Scope
Centralized capability validation in the syscall gate.

## Contract
Syscalls marked with `BH_SYSCALL_F_CAP_REQUIRED` must specify:
- `cap_arg_index`: The argument index containing the capability handle.
- `required_cap_type`: The expected type of the capability.
- `required_rights`: The bitmask of required rights.

## Failure Behavior
If validation fails, the handler is not called and `BH_ERR_ACCESS_DENIED` is returned.
