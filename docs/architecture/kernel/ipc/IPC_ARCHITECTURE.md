---
title: Kernel IPC Architecture
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Kernel IPC Architecture

This document aggregates the theoretical primitives for Inter-Process Communication (IPC) in Bharat-OS. The primary cross-core messaging mechanism moving forward is **URPC**. For details regarding that, see the `urpc/` directory. The primitives below define legacy or specialized IPC support.

## Shared Memory
Shared memory allows two or more processes to map the same physical memory pages into their respective virtual address spaces. In Bharat-OS, this is modeled via the Memory Protection Architecture (MPA) using capabilities to grant access.

## Message Queues
Message queues provide an asynchronous mechanism for passing discrete messages between processes. They are bounded in size and often implemented using kernel-allocated ring buffers.

## Pipes
Pipes provide a unidirectional, byte-stream communication channel, commonly used for parent-child process communication or chaining filters (like POSIX pipes).

## Semaphores
Semaphores are synchronization primitives used to coordinate access to shared resources or to signal events between processes. They can be counting semaphores or binary semaphores (mutexes).

## Signals
Signals are a form of asynchronous software interrupts used to notify a process that an event has occurred (e.g., termination request, memory fault).
