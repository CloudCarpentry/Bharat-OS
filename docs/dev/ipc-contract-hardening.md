# IPC/URPC Contract Hardening (Developer Branch)

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## Scope and intent

This change hardens the existing IPC contract path without introducing a parallel IPC model. It keeps layer ownership intact:

- Contract types remain in `include/bharat/uapi/ipc/`.
- User-facing API remains in `lib/ipc`.
- Kernel endpoint mechanism remains in `kernel/src/ipc/endpoint_ipc.c`.
- URPC transport remains in `lib/urpc` and kernel uRPC components.

## Step 0 audit inventory (reuse/extend/replace matrix)

| Path | Type / Function | Layer owner | Current usage | Decision |
|---|---|---|---|---|
| `include/bharat/uapi/ipc/contract.h` | `bharat_ipc_contract_header_t` | uapi | active, shared in contracts/validation | **Extend** (add `header_version`, `status` only). |
| `include/bharat/uapi/ipc/status.h` | IPC status aliases | uapi | active | **Reuse + extend aliases** (timeout/busy/endpoint). |
| `kernel/include/ipc_endpoint.h` | `ipc_endpoint_*`, `ipc_status_t` | kernel | active, core endpoint mechanism | **Reuse** (no redesign). |
| `kernel/src/ipc/endpoint_ipc.c` | `ipc_endpoint_create/send/receive` | kernel | active, capability-checked | **Reuse** (lib wraps syscall bridge). |
| `lib/include/ipc_user.h` | syscall wrappers for endpoint create/send/recv | lib bridge | active (syscall bridge) | **Reuse** for lib IPC transport path. |
| `lib/ipc/include/bharat/ipc/ipc.h` | `bharat_ipc_send/recv/call` | lib | stub API | **Extend and implement** (add timeout-aware `_ex` variants). |
| `lib/ipc/src/ipc.c` | IPC API implementation | lib | stub-only | **Implement** using existing endpoint syscall path. |
| `lib/urpc/urpc.[ch]` | `urpc_channel_t`, send/receive | transport/lib | active tests, standalone transport | **Reuse** unchanged. |
| `kernel/src/lib/ds/urpc_ring.c` | ring mechanism | transport/kernel | active | **Reuse** unchanged. |
| `kernel/include/bharat/urpc.h` + `kernel/src/urpc/*` | channel/bootstrap control | kernel transport | active/partial | **Reuse** unchanged (out of scope redesign). |

## Minimum missing contract identified

The branch already had:

- Endpoint capabilities and ownership enforcement in kernel IPC.
- URPC/channel and ring transport mechanisms.
- A service-level message contract header.

The missing pieces were primarily in `lib/ipc`:

- No functional send/recv/call implementation.
- No timeout-aware public helper surface.
- No strict mapping of kernel IPC failures to user-visible IPC contract statuses.
- No correlation enforcement for request/response in `ipc_call`.

## Additive uapi hardening

`bharat_ipc_contract_header_t` was **extended additively**:

- `header_version` for protocol/version checks.
- `status` for response/contract-level result propagation.

And canonical flags were normalized:

- `BHARAT_IPC_FLAG_REPLY`
- `BHARAT_IPC_FLAG_NONBLOCK`
- `BHARAT_IPC_FLAG_CAP_TRANSFER`

No duplicate header format was introduced.

## lib/ipc contract now provided

Implemented in `lib/ipc/src/ipc.c`:

- `bharat_ipc_send()` / `bharat_ipc_recv()` / `bharat_ipc_call()` (blocking wrappers).
- Timeout-aware variants:
  - `bharat_ipc_send_ex(..., timeout_ticks)`
  - `bharat_ipc_recv_ex(..., timeout_ticks)`
  - `bharat_ipc_call_ex(..., timeout_ticks)`

Behavior:

- Uses existing endpoint syscall bridge (`ipc_user.h`) by default.
- Validates header version and payload length.
- Frames contract header + payload into one endpoint message.
- Maps kernel endpoint errors to IPC contract status aliases.
- Enforces request/response correlation in `bharat_ipc_call_ex` by matching `message_id`.

## Defensive validation added

- Reject null/invalid headers.
- Reject payload length overflow against endpoint payload envelope.
- Reject malformed receive frames shorter than header.
- Reject truncated payload copies.
- Enforce correlation-id (`message_id`) match for call/reply.

## What is deferred (explicitly)

- Scheduler/PMM/VMM redesign.
- URPC ring layout or control-plane redesign.
- Full capability model redesign.
- Cross-core distributed auth protocol.
- Rich async session/channel abstraction in `lib/ipc`.

This preserves compatibility and avoids architecture drift / duplicate IPC models.
