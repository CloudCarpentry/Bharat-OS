---
title: Architecture Syscall ABI Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - syscall
see_also:
  - README.md
---
# Architecture Syscall ABI Matrix

## Status
Accepted

## Scope
Support matrix for syscall ABI across architectures.

## Per-Architecture Behavior
| Arch | Syscall Support | SDK Wrapper | Extractor |
|------|-----------------|-------------|-----------|
| x86_64 | Supported | Yes | Yes |
| arm64 | Supported | Yes | Yes |
| arm32 | Supported | Yes | Yes |
| riscv64| Supported | Yes | Yes |
| riscv32| Supported | Yes | Yes |

## Security Invariants
All architectures must implement `arch_trap_extract_syscall` and `arch_trap_set_syscall_return`.
