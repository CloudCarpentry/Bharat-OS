# Capability and IPC Baseline (v1)

This document summarizes the current kernel baseline for capability tables and IPC.

## Capability tables

- Per-process capability table (`capability_table_t`) allocated during `process_create`.
- Capability entries include:
  - object type,
  - object reference,
  - fine-grained rights (`SEND`, `RECEIVE`, `MAP`, `UNMAP`, `SCHEDULE`, `DELEGATE`).
- Delegation support enforces right reduction (`cap_table_delegate` requires delegated rights to be a subset of source rights).

## IPC models

### 1. Synchronous endpoint IPC

- Endpoint create API grants two capabilities:
  - sender capability (`CAP_PERM_SEND`),
  - receiver capability (`CAP_PERM_RECEIVE`).
- `ipc_endpoint_send` and `ipc_endpoint_receive` perform capability lookup + rights checks.
- Blocking behavior baseline:
  - send to full endpoint => `IPC_ERR_WOULD_BLOCK` and current thread marked blocked,
  - receive on empty endpoint => `IPC_ERR_WOULD_BLOCK` and current thread marked blocked.

### 2. Asynchronous URPC rings

- URPC ring init enforces minimum capacity and non-null backing storage.
- Send/receive validate ring configuration and return typed status codes (`URPC_ERR_INVALID`, `URPC_ERR_EMPTY`, `URPC_ERR_FULL`).

## AI scheduler control-plane IPC

- Kernel exposes capability-secured endpoint creation for governor control (`ai_kernel_create_governor_endpoint`).
- Incoming endpoint payloads are parsed as `ai_suggestion_t` and validated before enqueue (`ai_kernel_ingest_suggestion_ipc`).
- Scheduler consumes queued suggestions in timer/scheduling path, not directly in IPC receive path.
- Migration validation is bounded by available NUMA nodes and action type checks.

## User-space syscall API surface

- Endpoint create/send/receive are exposed as syscall IDs and wrapped by user header API (`lib/include/ipc_user.h`).

## Deferred for production

- Multi-receiver endpoint queues and wake-up lists.
- Cross-process capability transfer policy and revocation.
- Capability-backed shared-memory channels and zero-copy user rings.
