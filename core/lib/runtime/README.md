# lib/runtime

## Purpose
A minimal native runtime for Bharat services and apps. Defines the startup ABI, service main loop, bootstrap capability fetching, panic/assert/log helpers, and allocation/thread wrappers. Without this, every service reinvents the same scaffolding.

## Dependencies
- **May depend on:** `lib/cap`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `services/*`, `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers.

## Planned Public API
- `bharat_runtime_init`
- `bharat_runtime_shutdown`
- `bharat_runtime_get_bootstrap_cap`
- `bharat_runtime_log`
- `bharat_runtime_panic`
- `bharat_runtime_main_wrapper`

## Immediate TODOs
- Add real TLS/thread struct initialization.
- Integrate heap allocators.
- Wire `bharat_runtime_log` to actual debug serial or `logd` endpoints.
- Map the bootstrap capability retrieval to the appropriate syscall or root registry endpoint.

## Status
Stub.
