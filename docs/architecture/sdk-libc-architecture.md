# Bharat-OS SDK and Libc Architecture

For edge devices, drones, gateways, robotics nodes, and appliance-class systems, the winning move is not "full desktop POSIX first". The winning move is:

1. **Small, deterministic libc first**
2. **Selective POSIX compatibility second**
3. **Personality layers third**

This gives fast bring-up, app portability, and a clean architecture that does not drag monolithic Unix assumptions into the kernel.

---

## Why this matters now

Without libc + SDK:
* Every userspace service becomes special-case code.
* Third-party code porting is painful.
* Test harnesses are fragile.
* Build failures around missing headers/string/stdio keep recurring.
* Edge bring-up takes too much bespoke work.

With a real SDK:
* `services/netmgr`, `servicemgr`, drivers, and test apps stop reinventing runtime support.
* Common embedded and systems code can be compiled quickly.
* Utilities, benchmarks, and protocol stacks can be ported.
* Kernel/services can be validated with realistic apps.
* A path toward drone/edge appliances is created without waiting for a "full OS".

---

## The 3-Layer Design

This architecture is designed as **3 layers**, not one giant "POSIX support" blob. Do **not** hardcode POSIX semantics into the kernel. Keep the kernel capability-oriented and message-driven. Let libc/personality adapt POSIX semantics onto Bharat-OS primitives.

### 1. Core SDK libc/runtime
* C runtime (`crt0`)
* Memory / string / stdio basics
* `errno`, `time`, `malloc`, `env`, startup, standard headers
* Syscall veneer / service calls
* Freestanding + hosted modes

### 2. POSIX compatibility library
* File descriptors
* `read` / `write` / `open` / `close`
* Fork-free process/thread model initially (`spawn`-based)
* Signals subset
* Pthread subset
* Sockets shim (later)
* Termios minimal or stubbed at first

### 3. Personality compatibility
* Linux personality first
* Optional embedded/RT personality
* "Micro-POSIX" profile for drones/edge
* Subsystem personalities aligned with the multikernel direction

---

## The Target Model (Tiers)

We do **not** need full glibc-class compatibility now. We need **three compatibility tiers**.

### Tier A — SDK libc minimum viable platform
Enough to compile real services and embedded apps. Must include:
* `crt0` / process startup
* `stdlib.h`, `stdint.h`, `stddef.h`, `stdbool.h`, `stdarg.h`
* `string.h` (and optional `strings.h`)
* `stdio.h` (minimal)
* `errno.h`, `assert.h`, `ctype.h`, `limits.h`, `inttypes.h`
* `time.h` (minimal)
* `unistd.h`, `fcntl.h`, `sys/types.h`, `sys/stat.h` (subsets)
* `sys/mman.h` (subset or stubbed)
* `pthread.h` (very small subset or deferred)
* Heap allocator
* Thread-local `errno`
* Syscall/service-call wrapper layer

### Tier B — Embedded POSIX subset
Enough to port lightweight Unix-style applications. Must prioritize:
* `open` / `close` / `read` / `write` / `lseek`
* `dup` / `dup2`
* `pipe` or pipe-like channels
* `poll` / `select` (minimal)
* Directory ops, `stat` / `fstat`
* `clock_gettime`, `nanosleep`
* `getpid` / task id view
* `pthread_create` / `join` / `mutex` / `cond`
* `mmap` / `munmap` on top of VM abstractions
* `socket` API subset (only if network stack is ready)

**Avoid early obsession with:** full `fork`, full Unix `execve`, full signals, pty/tty complexity, locale/internationalization, and dynamic loader sophistication.

### Tier C — Personality compatibility
Expose a specific behavioral contract. This should live mostly above the kernel, not inside it.
* **Linux-like personality** (easiest code porting)
* **RT/embedded personality** (deterministic workloads)
* **Appliance profile** (edge/router/gateway)
* **Drone/autonomy profile** (stricter resource and fault contracts)

---

## Architecture Recommendations

### 1. Kernel should expose mechanisms only
Kernel exports: address spaces, threads/tasks, capability handles, endpoints/IPC, timers, VM map/unmap/protect, file/object handles, event/wait primitives, interrupt delivery, shared memory, and a per-core execution model.

The kernel should **not** directly promise a "POSIX process model" as its native truth.

### 2. Libc should sit on top of a stable UAPI
Create a **Bharat UAPI** that libc calls into. This gives clean boundaries: the kernel evolves internally, libc depends only on the stable UAPI, and POSIX stays a translation layer.

### 3. Services should provide Unix-like resources when needed
* VFS/file service backs the file descriptor model.
* Net service backs the socket personality.
* Process/service manager backs the spawn/exec-like model.
* Timer service backs clock/sleep APIs.
* Signal/emergency fault model mapped in runtime, not kernel-first.

---

## Libc Internal Structure

What libc should actually look like in Bharat-OS:

### A. `libbcrt` — C runtime
* Entry point
* Constructor/destructor handling
* TLS init
* `argv`/`env` setup
* Exit teardown
* Stack protector hooks (if enabled)

### B. `libbsys` — System interface layer
* Syscall stubs or endpoint IPC wrappers
* Handle/object abstraction
* Thread creation primitive
* Virtual memory requests
* Timer requests
* fd/object translation helpers

*This is the lowest stable user ABI.*

### C. `libc` — ISO C library
* Pure C functions
* `stdio`
* `malloc`/`free`
* String/memory routines
* Formatted I/O
* Time conversion basics
* Environment handling
* `errno`

### D. `libposix`
* `open` / `read` / `write` / ...
* `pthread_*`
* Path + stat wrappers
* Polling/event waiting
* Signals subset
* Process/task facade

### E. `libpersonality-linux`
* Linux errno behavior nuances
* Linux API quirks where needed
* Flags translation
* `/proc` expectations (via virtual service if chosen)
* Compatibility constants and structure behavior

---

## What Not To Do

1. **Do not make `fork` a day-1 requirement.** `fork` is expensive architecturally and infects everything. Treat full `fork()` as later, optional, or personality-specific. Use `spawn`, `posix_spawn`, exec-style launch, or clone-like threaded tasks if needed.
2. **Do not let libc talk to unstable internal kernel structures.** Everything should go through a narrow UAPI.
3. **Do not bury POSIX semantics in many services ad hoc.** Create a single compatibility ownership model.
4. **Do not promise full glibc desktop compatibility.** That will swamp the project.
5. **Do not overbuild heavy Unix ecosystems early.** Locale, i18n, iconv, NSS, PAM, etc., are not relevant for the immediate edge/drone target.
