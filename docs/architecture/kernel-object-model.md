# Kernel Object Model

## Overview

Bharat-OS operates on a strict, capability-based object model managed entirely within the trusted microkernel. To keep the Trusted Computing Base (TCB) minimal and verifiable, the kernel only understands a finite set of fundamental objects.

## Core Objects

1. **Threads (kthread_t)**: The fundamental unit of execution. The kernel schedules threads, not heavy processes.
2. **Tasks (ktask_t)**: A container for a resource domain. A task holds an address space (VType) and a Capability Space (CSpace).
3. **Capabilities (cap_t)**: Unforgeable tokens representing authority over an object. Every operation in Bharat-OS requires a valid capability.
4. **Endpoints (ep_t)**: IPC portals that threads use to send and receive synchronous messages.
5. **Frames (frame_t)**: Contiguous regions of physical memory allocated to a task.
6. **Interrupt Handlers (irq_t)**: Objects that allow a thread to register and wait for hardware interrupts.

## Lifecycle

All objects except the initial root task are dynamically created by user-space invoking the `Retype` operation on untyped memory capabilities. The kernel never implicitly allocates memory on behalf of user-space, ensuring deterministic execution for the Bharat-RT product line.
