# Bharat-OS Authenticated IPC Receive Envelope Review (IPC-P0-002)

**Review Date**: October 2026  
**Scope**: Kernel IPC, Native Syscall ABI, Capability Role Mapping, Fault-Safe Usercopy, UAPI Receive Envelope  
**Status**: **COMPLETED & HARDENED** (`BH-IPC-P0-002`)  
**Authority**: [AGENTS.md](../../AGENTS.md), [interface/contracts/abi/native_syscalls.json](../../interface/contracts/abi/native_syscalls.json), [docs/architecture/ipc-receive-envelope-architecture.md](../architecture/ipc-receive-envelope-architecture.md)  
**Branch / Commit**: `feature/ipc-authenticated-receive-envelope` (`ad3761935795f9efb9d4dfe435f49d6dd2e09f35`)

---

## 1. Executive Summary & Architectural Motivation

In a verified capability-based microkernel, untrusted userland message headers must never be trusted to assert process identity, rights, or authorization scopes. Prior to `BH-IPC-P0-002`, IPC endpoints relied on raw 56-byte wire messages without kernel-authenticated provenance, forcing privileged services (such as `vm_manager` and the supervisor) to operate behind defensive fail-closed policy shims (ADR-039).

Under ticket **`BH-IPC-P0-002`**, the architecture was hardened to provide genuine **kernel-authenticated provenance attestation**:
1. **Send-Time Provenance Capture**: When a message is accepted by `bh_sys_endpoint_send`, the kernel immediately captures the immutable identity of the sender (`sender_pid`, `sender_process_object_id`, `sender_cspace_id`, `sender_owner_core`) directly from the caller's verified kernel execution context (`bh_syscall_ctx_t` -> `process_t` -> `security_sandbox_ctx`).
2. **Versioned Fixed-Width Envelope**: Upon receive, the kernel delivers the sender's unauthenticated payload alongside a trusted 96-byte kernel-generated receive envelope (`bh_ipc_receive_envelope_v1_t`), with bitmask flags (`valid_fields`) certifying attested fields.
3. **Backward-Compatible Syscall Dispatch**: Native Syscall 9 (`BH_SYS_ENDPOINT_RECEIVE`) was extended with a selector scalar in register `arg[1]` (`BH_IPC_RECEIVE_SELECTOR_LEGACY` for 40-byte legacy carrier vs `BH_IPC_RECEIVE_SELECTOR_V1` for 64-byte versioned carrier descriptor).
4. **Single-Capability Role Classification**: Prevents privilege confusion by strictly inspecting transferred capabilities:
   - `CAP_TYPE_ENDPOINT` with `CAP_RIGHT_ENDPOINT_SEND` is installed as `reply_send_cap` (`BH_IPC_ENVELOPE_F_REPLY_SEND_CAP`).
   - Resource capabilities (memory, thread, device) are installed as `authority_cap` (`BH_IPC_ENVELOPE_F_AUTHORITY_CAP`).
   - Reply endpoints can never masquerade as service authority tokens.
5. **Fault-Safe Copyout Rollback**: If usercopy of payload, length, or envelope fails (`EFAULT`), newly installed capabilities in the receiver's CSpace are immediately revoked (`cap_table_revoke`), preventing handle leaks and dangling capability slots.

---

## 2. Invariants & Security Architecture Review

### 2.1 Wire Safety & Memory Layout Assertions
- Both the 96-byte receive envelope (`bh_ipc_receive_envelope_v1_t`) and the 64-byte receive descriptor (`bharat_sys_endpoint_receive_v1_args_t`) are fixed-width across all target ISAs.
- Compile-time `_Static_assert` (or C++ `static_assert`) validates `sizeof` and `offsetof` for every field in [interface/include/bharat/uapi/ipc/envelope.h](../../interface/include/bharat/uapi/ipc/envelope.h) and [interface/include/bharat/uapi/syscall_args.h](../../interface/include/bharat/uapi/syscall_args.h).
- Validated on host compilation with GCC/Clang and cross-compilation for x86_64, aarch64, riscv64, armv7-a, and riscv32.

### 2.2 TOCTOU Prevention & Usercopy Fault Safety
- The receiver provides pointers in user memory (`out_payload_ptr`, `out_len_ptr`, `out_envelope_ptr`).
- The kernel validates that write ranges do not overflow user address boundaries.
- Provenance is stored strictly in kernel memory (`ipc_endpoint_t` queue buffer) and copied directly to the user-provided envelope buffer via `bh_usercopy_to_user()`.
- If any usercopy fails after capability installation in receiver CSpace, `cap_table_revoke()` is immediately executed on the installed handle, leaving no orphaned capability in the receiver's CSpace.

### 2.3 Strict Truth Standard: Zero Unprovisioned Identity Fabrication
- The kernel does not yet possess cryptographic boot session or PM service incarnation tracking mechanisms.
- Rather than synthesizing dummy non-zero IDs, `boot_session_id` and `service_incarnation_id` are set to `0`, and flags `BH_IPC_ENVELOPE_F_BOOT_SESSION` and `BH_IPC_ENVELOPE_F_SERVICE_INCARNATION` remain strictly cleared.

---

## 3. Verification & Evidence Table

The changes were rigorously verified across host unit tests, guest boot selftests, and the full five-architecture QEMU matrix:

| Verification Gate | Command / Test Suite | Result | Evidence Observed |
|---|---|---|---|
| **Syscall ABI Check** | `python tools/abi/syscall_abi.py --check` | **PASS** | Hash matches `native_syscalls.lock.json` |
| **Agent / Layer Linters** | `python tools/agents/verify.py` | **PASS** | Agent config, layer references, display tiers pass |
| **Embedded Boards** | `python tools/nirmaan/cli.py verify --all-boards` | **PASS** | 4/4 board manifests qualified without MMIO clashes |
| **Host ABI Layout** | `quality/tests/host/test_uapi_abi.c` | **PASS** | `_Static_assert` validated in host CTest harness |
| **Host Lifetime & Interleaving** | `quality/tests/host/test_ipc_transfer_lifetime.c` | **PASS** | 12,000 cycles + `test_authenticated_envelope_lifecycle()` |
| **Usercopy Fault Safety** | `quality/tests/init/test_endpoint_usercopy.c` | **PASS** | Copyout fault rollback, negative spoof checks pass |
| **QEMU x86_64 Desktop** | `python tools/run_qemu_matrix.py` | **PASS** | `ktest_ipc_envelope_run PASS`, 17 markers, `BOOT_PASS` |
| **QEMU ARM64 Desktop** | `python tools/run_qemu_matrix.py` | **PASS** | `ktest_ipc_envelope_run PASS`, 17 markers, `BOOT_PASS` |
| **QEMU RISC-V 64 Desktop** | `python tools/run_qemu_matrix.py` | **PASS** | `ktest_ipc_envelope_run PASS`, 17 markers, `BOOT_PASS` |
| **QEMU ARM32 MMU-Lite** | `python tools/run_qemu_matrix.py` | **PASS** | `ktest_ipc_envelope_run PASS`, 17 markers, `BOOT_PASS` |
| **QEMU RISC-V 32 MMU-Lite** | `python tools/run_qemu_matrix.py` | **PASS** | `ktest_ipc_envelope_run PASS`, 17 markers, `BOOT_PASS` |

---

## 4. Remaining Production Blockers & Non-Claims

In strict obedience to [AGENTS.md](../../AGENTS.md), this review explicitly disclaims readiness for features that remain incomplete:

1. **Service Incarnation & Boot Session Generation**:
   Requires PM and bootstrap cryptographic token integration. `boot_session_id` and `service_incarnation_id` remain 0.
2. **Dual-Capability Transport**:
   IPC carries one capability per transmission. True RPC transactions requiring both a service authority token and an independent reply endpoint channel require Phase 2 multi-cap descriptors or badged endpoints.
3. **Privileged VM Manager & Supervisor RPC Authorization**:
   Privileged RPC entry points to `vm_manager` and supervisor remain fail-closed per ADR-039 until full process launch state integration is verified.
4. **Physical Cluster / Node uRPC**:
   Cross-node physical wire protocols remain Phase 3 work; verified today only on uniprocessor and SMP multi-core QEMU systems.

---

## 5. Changed Files Inventory & Diff Hygiene

All modifications adhere to repository hygiene rules (no third-party changes, zero generated configuration files checked into git):

```text
core/kernel/include/ipc_endpoint.h
core/kernel/src/ipc/endpoint_ipc.c
core/kernel/src/personality/native/native_syscall_handlers.c
core/kernel/src/tests/CMakeLists.txt
core/kernel/src/tests/ktest_ipc_envelope.c
core/lib/include/ipc_user.h
core/lib/ipc/include/bharat/ipc/ipc.h
core/lib/ipc/src/ipc.c
docs/architecture/CONTRACTS.md
docs/architecture/ipc-receive-envelope-architecture.md
docs/architecture/p0-ipc-envelope-proposal.md
docs/architecture/p0-ipc-identity-verification.md
docs/architecture/README.md
docs/reviews/README.md
docs/reviews/ipc-p0-002-authenticated-receive-envelope-review.md
interface/contracts/abi/native_syscalls.json
interface/contracts/abi/native_syscalls.lock.json
interface/include/bharat/uapi/ipc/envelope.h
interface/include/bharat/uapi/syscall_args.h
quality/tests/host/CMakeLists.txt
quality/tests/host/test_ipc_transfer_lifetime.c
quality/tests/host/test_uapi_abi.c
quality/tests/init/test_endpoint_usercopy.c
ROADMAP.md
```
