# Signals

## Overview
POSIX signals (`SIGINT`, `SIGKILL`, `SIGSEGV`) are a form of asynchronous inter-process communication that force a process to execute a specific signal handler function, preempting its normal execution flow.

## The Problem with Signals in Microkernels
In a monolithic kernel, signals are implemented by the kernel modifying the user-space stack of the target process to push a signal frame and changing the instruction pointer to the signal handler before returning to user space.

This violates the strict boundary of a microkernel:
-   The kernel should not know about user-space ABIs or stack layouts.
-   Asynchronous preemption of a thread to run arbitrary code is dangerous and complex (especially regarding reentrancy and locks).

## Implementation in Bharat-OS

Bharat-OS does not implement POSIX signals directly in the kernel. Instead, it relies on a combination of **Exception Handling**, **Asynchronous Event Capabilities**, and a **User-Space Signal Emulation Layer** (usually part of libc or a dedicated process manager).

### 1. Hardware Exceptions (e.g., `SIGSEGV`, `SIGILL`)
When a thread causes a hardware exception (e.g., a page fault or illegal instruction), the kernel catches the trap.
1.  **Fault Endpoint:** Every thread has a registered "Fault Endpoint" capability in its TCB.
2.  **Kernel Action:** Instead of killing the thread immediately, the kernel suspends the thread and sends a synchronous IPC message containing the fault details (faulting address, instruction pointer) to the Fault Endpoint.
3.  **Process Manager:** A supervisor process (like an init daemon or debugger) listening on that endpoint receives the message. It can then decide to kill the thread, log the error, or, if it's emulating POSIX, inject a `SIGSEGV` signal into the thread's execution context.

### 2. Software Signals (e.g., `SIGINT`, `SIGTERM`)
When Process A wants to send `SIGKILL` to Process B:
1.  **IPC Request:** Process A sends an IPC message to the Process Manager (which holds the TCB capabilities for all processes).
2.  **Process Manager Action:** The Process Manager receives the request: "Kill Process B".
3.  **Kernel Action:** The Process Manager uses its `CAP_TYPE_TCB` capability for Process B's threads to invoke a `Suspend` or `Destroy` operation on them, effectively terminating the process.

### 3. POSIX Signal Emulation (libc)
To fully emulate POSIX `signal()` and `sigaction()`, the C library must implement a signal thread.
1.  When a process starts, libc spawns a hidden background thread.
2.  This thread blocks on a dedicated "Signal Endpoint" waiting for messages from the Process Manager.
3.  When the Process Manager wants to deliver `SIGUSR1`, it sends an IPC message to the Signal Endpoint.
4.  The background thread wakes up, identifies the target application thread, uses a capability to suspend the application thread, modifies its user-space registers/stack to point to the registered signal handler, and resumes it.

**Async-Signal Safety:** Because this emulation is complex, the POSIX requirement that signal handlers only call "async-signal-safe" functions (like `write()`, not `printf()` or `malloc()`) is strictly enforced by the potential for deadlock if the interrupted thread was holding a libc mutex.