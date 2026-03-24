# Kernel Boundary + Syscall ABI Freeze Plan (Execution Ticket)

## Objective
Freeze a personality-neutral kernel boundary that is stable for capability services and BIDL contracts.

## Scope (implemented in this change)
1. Canonical syscall numbering table under `include/bharat/uapi/`.
2. Stable ABI scalar IDs (`handle`, `object_id`, `cap_id`).
3. Three-layer error/status model:
   - Kernel internal status (`kstatus_t`) in kernel-only headers.
   - Syscall boundary errno model (`sys_errno_t`) in UAPI.
   - Service/BIDL contract status (`bharat_status_t`) in UAPI.
4. Strict syscall argument structs for complex syscall payloads.
5. ABI drift tests (compile-time assertions + runtime sanity).

## BIDL + Capability alignment rules
- BIDL messages must use stable width IDs (`u64` for handles/object/cap IDs) to match `bharat/uapi/abi_types.h`.
- Service responses must use `bharat_status_t`/mapped aliases for contract-level status, not syscall errno values.
- Capability transfer-related syscall payloads must remain struct-based and append-only.

## Next execution steps
1. Route any remaining ad-hoc syscall callsites to `bharat/uapi/syscall_args.h` carriers.
2. Add generated BIDL code checks to assert status type (`bharat_status_t`) and ID widths.
3. Gate CI on `test_uapi_abi` and fail on ABI drift unless ABI version policy is explicitly bumped.
