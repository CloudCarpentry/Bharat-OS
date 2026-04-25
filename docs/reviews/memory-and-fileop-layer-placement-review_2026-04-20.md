---
title: Memory + File Operation Layer Placement Review (memset/memcpy/memmove/open/openat)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Memory + File Operation Layer Placement Review (memset/memcpy/memmove/open/openat)

Date: 2026-04-20  
Scope: placement consistency for low-level memory and file-open operations across `core/arch/`, `core/hal/`, `core/kernel/`, `lib/`, and SDK-facing layers.

## 1) Executive summary

The codebase already has **good architectural intent** for layered placement:
- `core/arch/` exposes explicit memory-op dispatch contracts (`arch_memcpy/arch_memset/arch_memmove`) and scalar safe fallbacks.
- `lib/string` provides portable, kernel-independent baseline implementations.
- architecture docs/ADR define mechanism vs policy boundaries and explicitly require optimized hardware paths behind stable interfaces.

However, the implementation is currently **partially fragmented** in two areas:
1. **Memory operations duplication** in leaf libs (`internal_memcpy/internal_memset` in multiple files) instead of using one canonical layered entrypoint.
2. **File open path split-brain**: kernel VFS exports compatibility stubs while service and lib layers implement/forward real behavior, creating temporary duplicate API ownership and ambiguity around where `open/openat` belongs.

Overall rating: **Medium risk now, high payoff if unified in one pass**.

---

## 2) What is currently in-tree (fact snapshot)

### 2.1 Memory ops

- `lib/string/string.c` defines portable `memcpy`, `memset`, `memmove` (SDK/shared reusable C implementation).
- `core/arch/common/memops_scalar.c` defines kernel-safe scalar fallbacks and tier-0 raw memset (`arch_memset_raw`) with anti-recursion guidance.
- `core/kernel/include/core/arch/memops.h` already provides a context-flagged dispatch contract for `arch_mem*` and safe scalar variants.
- `tools/check_no_recursive_memops.py` validates no recursive calls in memops symbols in compiled ELF.
- Some libs still define local duplicate helpers:
  - `lib/transport/loopback.c` (`internal_memcpy` + `internal_memset`)
  - `lib/msg/wire.c` (`internal_memset`)

### 2.2 File open/openat path

- `core/kernel/src/fs/file.c` currently provides **compatibility stubs** returning `K_ERR_REQUIRES_FS_SERVICE` for open/read/write/close APIs.
- `core/services/system/filesystem/fd_mgr.c` currently holds actual open/read/write/close logic and exports `vfs_open*` functions.
- `lib/fs/fs_client.c` directly forwards `fs_open/fs_read/...` to those service-side `vfs_*` symbols.
- `lib/runtime/host/runtime_host.c` has a separate runtime API (`bharat_runtime_file_open`) but it is still unimplemented.
- There is **no `openat` implementation** in core/kernel/lib/service path yet, so introducing it now without placement cleanup would likely extend inconsistency.

---

## 3) Recommended layer ownership (target model)

## 3.1 `memset/memcpy/memmove`

### `core/arch/`
Own:
- ISA-specific mechanisms and instruction-level fastpaths.
- context-sensitive dispatch implementation details (e.g., IRQ-safe / early-boot constraints).

Do not own:
- API policy about when user-space or services should call memory ops.

### `core/hal/`
Own:
- optional architecture-neutral memory-op contract surface if needed for cross-platform modules (only if more than kernel uses it).
- capability discovery translation (what fastpaths are available) from core/platform/arch to unified contract.

Do not own:
- board-specific tuning tables and service policy.

### `core/kernel/`
Own:
- canonical kernel memory-op entrypoints for ring-0 use (with flags for context safety).
- no-recursion safety guarantees and trap/boot-safe fallback enforcement.

Do not own:
- user SDK ABI for libc `memcpy/memset` semantics.

### `lib/` + SDK
Own:
- canonical portable C baseline used by user-space, services, and host tools.
- optional IFUNC/pluggable dispatch in libc later, but still ABI-stable and independent from kernel-private state.

Do not own:
- ad-hoc per-file duplicate `internal_mem*` loops except in tightly justified bootstrapping files.

**Rule of thumb:**
- If the caller is kernel internal and context-sensitive -> `arch_mem*` path.
- If the caller is shared/user SDK code -> `lib/string` path.

## 3.2 `open/openat`

### `core/arch/` and `core/hal/`
- Should not own file-open semantics. No `open/openat` policy/mechanism here.

### `core/kernel/`
Own:
- mechanism contracts only: capability checks, descriptor/token model definitions, IPC/UAPI syscall entry points.
- no file-system global policy or direct mount namespace policy logic beyond mechanism boundaries.

### `core/services/system/filesystem`
Own:
- concrete path resolution, mount namespace logic, FD table policy, and implementation orchestration.
- service-side implementation of `open/openat` semantics over kernel mechanisms.

### `lib/fs`, runtime, SDK POSIX layer
Own:
- public API mapping (`open/openat`) -> UAPI/IPC contracts.
- personality/compat behavior translation and errno mapping.

**Rule of thumb:**
- Kernel exports mechanism; FS service owns behavior; SDK/libc owns compatibility surface.

---

## 4) Concrete inconsistency list to address

1. **Duplicate memops in libs** (`internal_memcpy/internal_memset` in multiple files) risk drift and hidden performance cliffs.
2. **Dual ownership naming collision** (`vfs_open*` in kernel stubs and service real implementation) makes boundaries unclear and risks accidental link-order coupling.
3. **Missing `openat` canonical entrypoint** means each layer may implement ad-hoc variants if added prematurely.
4. **Runtime host file API path is disconnected** (`bharat_runtime_file_open` unimplemented), leaving two parallel file-open API families.

---

## 5) Proposed unification plan

### Phase A — memory ops normalization

1. [x] Define two canonical facades and ban others:
   - `arch_mem*` for core/kernel/ring-0 context-sensitive use.
   - `mem*` from `lib/string` (or `freestanding_string.h`) for shared/lib/sdk use.
2. [x] Replace per-file `internal_mem*` duplicates with one selected facade per layer.
3. [x] Keep `tools/check_no_recursive_memops.py` in CI as a hard guard.
4. [x] Add a placement lint rule: disallow new `internal_memcpy/internal_memset` symbols outside explicitly allowlisted files.

### Phase B — open/openat ownership cleanup

1. [x] Freeze ownership model:
   - kernel `vfs_*` stays mechanism bridge only.
   - service filesystem owns real open logic.
2. [x] Rename service implementation surface to service-specific names (e.g., `fsd_open_file`) to remove ambiguity with kernel shim names.
3. [x] Introduce one `openat` contract at SDK/UAPI boundary first, then map into FS service implementation.
4. [x] Make `lib/fs` and runtime host path converge on same internal client transport contract.

### Phase C — guardrails

1. [x] Extend layer-reference checks to enforce:
   - no HAL/arch includes from SDK/user-facing lib unless explicitly permitted.
   - no direct service implementation symbol imports from generic lib APIs (must go through contract header).
   - *Status: Heuristic-based linter implemented in `tools/lint/check_layer_references.py`.*
2. [x] Add architecture conformance test matrix for memops behavior equivalence and overlap safety.
   - *Status: `core/kernel/src/quality/tests/ktest_memops.c` added to validate `arch_mem*` primitives.*

---

## 6) Suggested placement matrix (quick reference)

| Operation category | Arch | HAL | Kernel | Service | Lib/SDK |
|---|---|---|---|---|---|
| `memcpy/memset/memmove` baseline | optimized mechanism | optional contract/caps | kernel context facade | N/A | canonical portable libc facade |
| IRQ/boot-safe memory ops | yes | maybe capability query | yes (must call arch-safe path) | no | no |
| `open/openat` syscall/UAPI mechanism | no | no | yes (mechanism contract) | consumes | wraps for apps |
| Path policy / mount semantics | no | no | minimal mechanism only | yes (owner) | adapter only |
| POSIX compatibility (`openat` errno semantics) | no | no | no | maybe policy input | yes (owner) |

---

## 7) Final recommendation

Implement **one-time boundary hardening** now before scaling openat and further hardware-accelerated memops:
- unify memory operations into two sanctioned paths (kernel-arch vs lib-sdk),
- move file-open behavior ownership clearly to FS service,
- define `openat` once at the SDK/UAPI contract, not ad-hoc per layer,
- enforce by lint/CI so the codebase does not regress into duplicate “spaghetti” helpers.

This gives consistency, better hardware leverage, and avoids duplicated/entangled implementations.
