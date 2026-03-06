# ADR-004: Linux Compatibility as a Deferred Personality

## Status

Accepted

## Context

A brand-new operating system has a classic "chicken-and-egg" problem: nobody will use it if it lacks software, but nobody writes software for an OS with zero users. Initially, the architectural ambition was a "compatibility-everything" layer supporting Linux, Windows NT, and BSD simultaneously.

This scope is an architectural "boss fight" and practically guaranteed to fail due to conflicting semantics (e.g., Windows asynchronous I/O vs. Linux epoll).

## Decision

We established a strictly phased compatibility goal:

1. **Phase 1 Target**: POSIX-Native (focusing on libc).
2. **First Compatibility Target (Research Phase)**: **Linux Subsystem**.

We explicitly deferred Windows and BSD. We selected Linux as the primary future compatibility subsystem because the open-source infrastructure running the cloud primarily relies on Linux ABI (Application Binary Interface) semantics.

## Consequences

### Positive

- **Scope Realism**: We are not attempting to build an unmaintainable Frankenstein's monster OS spanning every ABI in existence.
- **Clear Roadmap**: The engineering team targets robust POSIX first, which naturally lays the groundwork for Linux subsystem mapping later.

### Negative

- **Adoption Friction**: Early adopters cannot simply run existing proprietary container images on Day 1 without strict POSIX compliance or static recompilation via the Unikernel toolchain.

## Technical Implementation of POSIX Compatibility

To support cross-compilation and ensure a robust POSIX compatibility layer (the "Linux Personality"), we have adopted a strategy of using conditional guards for core POSIX types in our internal headers (e.g., `lib/posix/include/unistd.h`).

Types such as `ssize_t`, `pid_t`, and `off_t` are wrapped in `#ifndef` guards (e.g., `_SSIZE_T_DEFINED_`). This prevents redefinition conflicts between Bharat-OS internal headers and the standard libraries provided by cross-compilation toolchains, allowing us to maintain a clean microkernel abstraction while seamlessly supporting the Linux ABI.
