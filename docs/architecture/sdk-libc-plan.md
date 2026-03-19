# Bharat-OS SDK and Libc Implementation Plan

This document outlines the phased implementation plan for the Bharat-OS SDK and libc layer, transforming the kernel into a testable OS platform.

## Phase 0 — Define ABI and Profiles

**Goal:** Establish the foundational contracts before writing code.

* **Deliverables:**
  * Bharat user ABI spec
  * libc support matrix
  * POSIX subset matrix
  * Personality compatibility matrix
  * Error code mapping
  * Handle/fd model
  * Startup ABI: `argv` / `env` / `auxv` / TLS
* **Output Documents:**
  * `docs/architecture/sdk-libc-plan.md` (This document)
  * `docs/architecture/posix-compat-matrix.md`
  * `docs/architecture/personality-model.md`

## Phase 1 — Minimal SDK Libc

**Goal:** Build a usable SDK for Bharat-OS native services and tests.

* **Implement:**
  * `crt0`
  * `libbsys`
  * `errno`
  * `malloc` / `free`
  * `memcpy` / `memset` / `memmove` / `strcmp` / ...
  * Minimal `printf` / `stdio`
  * `abort` / `exit` / `assert`
  * `time` and `clock_gettime` (minimal)
  * `read` / `write` / `open` / `close` wrappers (if backend exists)
  * Header package for cross-compilation
* **Definition of Done:**
  * Services compile without hacking missing standard headers.
  * Test apps can print/log, allocate memory, sleep, and access basic handles.
  * Toolchain can target Bharat-OS cleanly.

## Phase 2 — Hosted Libc + fd Model

**Goal:** Introduce a robust file descriptor and I/O model.

* **Implement:**
  * Userspace fd table or kernel fd table contract
  * Stdio streams over fd model
  * Path-based `open` / `stat` APIs
  * Directory iteration
  * `mmap` / `munmap` / `protect`
  * `poll` / `select` (minimal)
  * Environment and startup polish
* **Definition of Done:**
  * Simple Unix-style tools compile.
  * Service processes stop using one-off runtime code.
  * VFS tests and libc tests run end-to-end.

## Phase 3 — Pthread and Synchronization Subset

**Goal:** Enable multithreaded services and portable code.

* **Implement:**
  * `pthread_create` / `join`
  * `pthread_mutex_*`
  * `pthread_cond_*`
  * TLS
  * `pthread_once` / init
  * Semaphores (optional)
  * Thread naming and priority hooks
* **Definition of Done:**
  * Common libraries with pthread subset build.
  * Network/control-plane services can use portable threading abstractions.

## Phase 4 — POSIX Profile for Edge/Appliance

**Goal:** Support networking and lightweight daemons.

* **Implement:**
  * Socket layer (if net stack is ready)
  * DNS / basic resolver path
  * `epoll` / `poll` personality choice
  * `posix_spawn`
  * Signals (minimal)
  * Logging/syslog compatibility stub or service
* **Definition of Done:**
  * Lightweight network daemons can be ported.
  * Router/gateway-class services can run.

## Phase 5 — Drone/RT Profile

**Goal:** Ensure determinism and strict resource control.

* **Implement:**
  * Monotonic deterministic timing
  * Affinity/priority APIs
  * Watchdog hooks
  * Bounded allocator mode
  * Capability-scoped process profile
  * Fail-fast runtime mode
* **Definition of Done:**
  * Flight-control-adjacent support code can run in a controlled userland profile.
  * Runtime behavior is analyzable and resource-bounded.

## Phase 6 — Linux Personality Widening

**Goal:** Improve broad compatibility (Later stage only).

* **Implement Selectively:**
  * Wider ioctl surface
  * More errno/flags quirks
  * `procfs` personality (if useful)
  * Fuller socket semantics
  * Shared library/runtime loader maturity

---

## Suggested Repo Structure

A clean layout to support this phased rollout:

```text
sdk/
  include/
    stdio.h
    stdlib.h
    string.h
    ...
  crt/
    crt0.c
    crt_arch_*.S
  libsys/
    syscall.c
    ipc_calls.c
    handles.c
    time.c
    vm.c
    thread.c
  libc/
    string/
    stdio/
    stdlib/
    errno/
    time/
    malloc/
  libposix/
    unistd/
    fcntl/
    stat/
    dirent/
    pthread/
    poll/
    mmap/
    spawn/
  personalities/
    linux/
    embedded/
    appliance/
    drone_rt/
```

---

## Concrete Next-Task Board

To immediately execute this plan, the following sequence of tasks should be completed:

* [ ] **Task 1:** Define Bharat user ABI and syscall/service boundary.
* [ ] **Task 2:** Create `sdk/include` and minimal CRT.
* [ ] **Task 3:** Implement `libsys` for thread, vm, time, IPC/wait, and object/handle ops.
* [ ] **Task 4:** Implement libc core: string, stdlib, errno, assert, minimal stdio, malloc.
* [ ] **Task 5:** Create POSIX subset matrix and mark each API: native, shim, stub, deferred.
* [ ] **Task 6:** Implement fd/object model over VFS/service handles.
* [ ] **Task 7:** Add pthread minimal layer.
* [ ] **Task 8:** Add `posix_spawn` style launcher before even thinking about `fork`.
* [ ] **Task 9:** Define Linux personality mapping document.
* [ ] **Task 10:** Create appliance and drone runtime profiles.
