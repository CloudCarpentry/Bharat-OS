# Contributing to Bharat-OS

Thank you for your interest in contributing to Bharat-OS! This document explains our standards, conventions, and how to get started.

---

## Design Philosophy

Bharat-OS is built on three non-negotiable pillars:

1. **Verification-First** — Every line added to the kernel's Trusted Computing Base (TCB) must be justifiable for future formal verification. Keep it small and provable.
2. **Policy Out, Mechanism In** — The kernel provides only primitives. All scheduling policy, memory heuristics, and OS personalities live in user-space servers.
3. **Incremental milestones** — Ship a working, bootable kernel before adding complexity. Experimental features live in `kernel/include/experimental/` until they are ready.

---

## Repository Layout

```
kernel/
  include/         # v1 stable public kernel headers
    experimental/  # Research-horizon headers (NOT built by default)
      personality/ # Compatibility OS stubs (Windows NT, Darwin, ...)
      fs/          # Advanced future filesystems (BFS, DFS)
  src/             # Kernel C source files
docs/
  architecture/    # Architecture Decision Records (ADRs) & design docs
  adr/             # Numbered ADR files
lib/               # Shared userspace libraries
subsys/            # User-space servers (VFS, net, driver host, ...)
tools/             # Build helpers and scripts
```

---

## Language Guidelines

| Component          | Language             | Rationale                                 |
| ------------------ | -------------------- | ----------------------------------------- |
| Kernel core (TCB)  | **C**                | Toolchain stability, verification tooling |
| User-space drivers | **Rust** (preferred) | Memory safety without a GC                |
| User-space servers | **Rust** or **C**    | Flexibility                               |
| Build/CI scripts   | Python / Shell       | Portability                               |

> Avoid C++ in the kernel. Template instantiation and RTTI make the binary surface hard to verify.

---

## Coding Standards

- **C style:** Follow Linux kernel style (8-space tabs). Run `clang-format` before committing.
- **Rust style:** `cargo fmt` and `cargo clippy --deny warnings`.
- **No global mutable state** in the kernel unless protected by a clear ownership discipline (spinlock + comment explaining why).
- All kernel functions that can fail **must return an error code** — never silently discard errors.
- Header guard format: `BHARAT_<SUBSYSTEM>_H` (e.g., `BHARAT_MM_H`).

---

## Working With Experimental Features

Headers in `kernel/include/experimental/` are **not compiled as part of the default kernel build**. They represent research work in progress.

- Do **not** `#include` experimental headers from stable kernel headers.
- Each experimental header **must** contain an `[EXPERIMENTAL]` comment at the top.
- To integrate an experimental feature into the core, open an ADR and get it reviewed first.

---

## Submitting Changes

1. **Fork** the repository and create a feature branch: `git checkout -b feat/my-feature`.
2. **Write tests** for any new allocator, IPC path, or capability primitive.
3. Ensure the kernel **cross-compiles cleanly** for both `x86_64` and `riscv64` QEMU targets (CI will check this automatically).
4. Open a **Pull Request** with a clear description linking to any relevant ADR or issue.
5. All PRs require at least one review before merging.

---

## Reporting Issues

Please open a GitHub Issue with:

- Your host OS and toolchain version
- Target architecture (`x86_64` / `riscv64`)
- Steps to reproduce
- Expected vs. actual behaviour

---

## License

By contributing, you agree that your contributions will be licensed under the MIT License as specified in the [LICENSE](./LICENSE) file, and that all usage is subject to the laws of the Republic of India.
