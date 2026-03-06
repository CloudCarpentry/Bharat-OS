# Personality Layer Model

## Overview

The Bharat-OS microkernel provides _no_ POSIX system calls natively. Features like `fork()`, `open()`, network sockets, and user accounts do not exist in Ring-0.

Instead, the OS relies on user-space Personality Layers to present expected environments to legacy or cloud applications.

## 1. POSIX-Native (Target API)

The primary execution environment for Bharat-OS native applications. It implements a POSIX-compliant C Library (`musl` or `libc`) where standard system calls (e.g., `read()` or `write()`) are intercepted by the library and translated directly into synchronous capability IPC calls directed at the VFS or Network servers.

## 2. Unikernel / Library OS (Cloud-Native Mode)

_Optional single-address-space mode for trusted workloads._
In this mode, a single application (e.g., an AI microservice or Nginx) is statically compiled _with_ the Bharat-OS core and the POSIX-Native translation layer into a single executable image.

- There is no user-space separation.
- The application invokes hardware drivers via direct function calls instead of IPC, eliminating all overhead.
- Strictly for Cloud virtualization where the Hypervisor already provides hardware isolation.

## 3. Compatibility Subsystems (Research Horizon)

Subsystems exist to run pre-compiled binaries unmodified.
**Linux Subsystem**:

- A user-space daemon (the Translator) traps Linux syscalls (`syscall` instruction or `int 0x80`) from the unmodified Linux executable.
- The Translator maps the Linux semantics into Bharat-OS capability RPCs.
- _Note: This is considered a deferred research module due to the enormous surface area of the Linux Syscall interface._
