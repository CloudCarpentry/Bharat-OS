# Formal Verification Scope

## Overview

The "Security-First" claim of Bharat-OS is backed by an architecture designed to be mathematically proven correct using formal methods (similar to the L4.verified project for seL4).

## The Trusted Computing Base (TCB)

Formal verification cannot prove an entire operating system, including billions of lines of GPU and network drivers, is flawless. We must radically shrink the TCB.

In Bharat-OS, the TCB consists _only_ of:

1. The kernel trap/interrupt entry and exit assembly stubs.
2. The Capability capability derivation and revocation logic.
3. The IPC message-copying mechanisms.
4. The low-level Thread context-switching scheduler.
5. The Virtual Memory page-table mapping logic inside the kernel.

**Estimated Size**: ~10,000 to 15,000 lines of C/Assembly code.

## Verification Constraints

To achieve a machine-checked proof (e.g., via Isabelle/HOL):

- The kernel uses **no dynamic memory allocation (`malloc`/`free`)**. All memory tracking structures for kernel objects are derived safely from capability `Retype` operations triggered by user-space.
- The kernel uses **no unbounded loops**. Every operation executes in a mathematically predictable time frame.
- **No blocking**. The kernel never sleeps on a lock; threads are merely suspended and inserted into scheduling queues.

## Scope Exclusion

We explicitely **do not** formally verify:

- Any file system (BFS, VFS).
- Network stacks (TCP/IP).
- ML-Heuristic Daemons.
- Linux Subsystem translating daemons.
  These are considered untrusted user-space applications and are strictly contained by the capability system enforced by the verified TCB.
