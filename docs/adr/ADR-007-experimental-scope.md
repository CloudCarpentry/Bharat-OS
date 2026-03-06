# ADR-007: Scope Restriction — Experimental vs. v1 Core

**Date:** 2026-03-06
**Status:** Accepted
**Author:** Divyang Panchasara

---

## Context

Following an external architectural review of Bharat-OS, it was noted that the kernel's `include/` directory contained headers for features that belong to entirely different development phases. Specifically:

- **RDMA / CXL cluster fabric** — requires hardware not available in QEMU test environments.
- **BFS / DFS** — advanced CoW/distributed filesystems far beyond a v1 bring-up need.
- **Unikernel mode** — an optional cloud-only deployment model, not required for microkernel correctness.
- **Windows NT & Darwin personalities** — enormous compatibility surfaces with hundreds of syscalls; research-horizon only.
- **Distributed Shared Memory (DSM)** — depends on RDMA and CXL; not a bring-up concern.

Mixing these headers into `kernel/include/` with the stable v1 core creates confusion about what is actively maintained, increases the cognitive load for new contributors, and risks accidental inclusions from verified kernel code paths.

---

## Decision

All headers and source files that are **not needed to boot and run a basic Bharat-OS kernel on QEMU (x86_64 or riscv64)** are moved to `kernel/include/experimental/`.

The canonical experimental directories are:

```
kernel/include/experimental/        # General research headers
kernel/include/experimental/fs/     # Advanced filesystems (BFS, DFS)
kernel/include/experimental/personality/  # OS compatibility layers
```

### Rules for `experimental/`

1. No stable v1 kernel header may `#include` a file from `experimental/`.
2. Every experimental header must carry a top-of-file `[EXPERIMENTAL]` comment.
3. Experimental code is **not compiled** by the default CMake build.
4. Promotion from `experimental/` to the v1 core **requires a new ADR**.

### Personality Ordering

Compatibility personality layers are prioritised as follows:

| Priority | Personality                   | Status                      |
| -------- | ----------------------------- | --------------------------- |
| 1        | **Linux syscall subsystem**   | Deferred — research horizon |
| 2        | POSIX-Native (musl/libc shim) | v1 design, user-space       |
| 3        | Unikernel Library-OS          | Experimental                |
| 4        | Darwin/macOS (Mach/BSD)       | Experimental stub           |
| 5        | Windows NT                    | Experimental stub           |

---

## Consequences

- **Positive:** Kernel include tree is clean; easy to identify what is in scope for v1 verification.
- **Positive:** New contributors can work on the core without being confused by RDMA or BFS headers.
- **Positive:** Experimental headers are preserved and ready for future development phases.
- **Negative:** Any external tooling that was `#include`-ing these headers directly from `kernel/include/` will need its paths updated to `kernel/include/experimental/`.
