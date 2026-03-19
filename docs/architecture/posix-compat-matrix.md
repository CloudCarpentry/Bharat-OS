# POSIX Compatibility Matrix

POSIX compatibility is a profile, not an all-or-nothing guarantee. In Bharat-OS, POSIX is implemented via a multi-tiered compatibility strategy aimed at fast bring-up, deterministic edge deployment, and minimal monolithic assumptions in the core kernel.

We define three profiles of POSIX support:

* **Profile P0 — Bring-up POSIX**: For building core services and tests. Includes file I/O basics, memory allocation, time/sleep, environment, stdout/stderr, minimal threading, path operations, error codes, and handle/event polling.
* **Profile P1 — Edge Appliance POSIX**: For gateways, routers, and industrial nodes. Adds sockets, basic DNS resolving, `select`/`poll`/`epoll`-like abstractions, daemon/service launching, config files, shared memory, and robust threading.
* **Profile P2 — Drone/Autonomy POSIX**: For deterministic, resource-bounded deployments. Adds monotonic time correctness, bounded allocator modes, priority/affinity APIs, lock-free IPC wrappers, memory locking subsets, watchdog integration, and strict fault containment contracts. Crucially, desktop POSIX features are excluded or restricted here.

## Support Labels

We use explicit support labels to classify how an API maps to the underlying multikernel architecture:

* **Native**: Backed directly by a Bharat-OS kernel or core service abstraction.
* **Shim**: Emulated in the runtime/service compatibility layer without corresponding kernel mechanisms.
* **Stub**: Declared to appease build systems, but strictly returns an error indicating it is not supported.
* **Deferred**: Not yet implemented, but planned for a future phase.
* **Forbidden**: Intentionally not supported or blocked in some profiles to maintain performance, determinism, or capability boundaries.

## Compatibility Matrix

The following matrix documents our intended support across the three primary API profiles.

| API Area | Profile P0 (Bring-up) | Profile P1 (Edge) | Profile P2 (Drone/RT) | Notes |
| --- | --- | --- | --- | --- |
| **ISO C Core** | Native | Native | Native | Mandatory foundation |
| **stdio Basic** | Native | Native | Native | Line-buffered is sufficient initially |
| **File I/O** | Native/Shim | Native | Native | Backed by VFS/object handles |
| **pthread Basic** | Deferred -> Native | Native | Native Subset | Avoid huge pthread surface early on |
| **fork** | Deferred | Deferred | **Forbidden** | Replaced entirely by `spawn` |
| **exec / spawn** | Shim/Native | Native | Native | Preferred process launch model |
| **mmap** | Native Subset | Native | Native Bounded | Backed securely by VM regions |
| **Sockets** | Deferred | Native (Later) | Optional Subset | Dependent on network stack readiness |
| **Signals** | Stub/Minimal | Minimal | Restricted | Avoid overbuilding early |
| **poll / select** | Shim/Native | Native | Native | Based entirely on waitable objects/endpoints |
| **termios** | Stub | Optional | N/A | Not an urgent priority |
| **Dynamic Loader** | Deferred | Later | Optional | Rely on static linking first |

This matrix must be maintained and updated as capabilities are merged into the SDK and libc tiers to ensure we maintain an accurate contract for developers and system builders.
