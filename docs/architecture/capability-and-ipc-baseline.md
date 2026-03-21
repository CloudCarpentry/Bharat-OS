# Capability and IPC Baseline (v1)

This document summarizes the current kernel baseline for capability tables and IPC.

## Capability tables

- Per-process capability table (`capability_table_t`) allocated during `process_create`.
- Capability entries include:
  - object type,
  - object reference,
  - fine-grained rights (`SEND`, `RECEIVE`, `MAP`, `UNMAP`, `SCHEDULE`, `DELEGATE`).
- Capability transfer policies: A strict set of rules evaluating transferable types and transferable rights attenuation. Only explicit capabilities carrying `DELEGATE` right can be transferred across domains.
- Delegation support enforces right reduction (`cap_table_delegate` requires delegated rights to be a subset of source rights).
- Revocation paths follow tree structures to gracefully revoke derived and explicitly granted capabilities recursively.

## IPC models

### 1. Synchronous endpoint IPC

- Endpoint create API grants two capabilities:
  - sender capability (`CAP_PERM_SEND`),
  - receiver capability (`CAP_PERM_RECEIVE`).
- `ipc_endpoint_send` and `ipc_endpoint_receive` perform capability lookup + rights checks, and gracefully check cross-process capability transfer parameters in a safe and atomic manner.
- Synchronous queues guarantee correct multi-waiter message isolation and atomic IPC capability installation directly into the recipient's capability table without side-channels.
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

- Capability-backed shared-memory channels and zero-copy user rings.
