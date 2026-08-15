# BharatLibC Architecture

BharatLibC is a next-generation, profile-driven, isolated standard library designed specifically for Bharat-OS userspace and standalone environments.

## Core Design Principles

1. **Strict Isolation**: BharatLibC relies only on stable, versioned public UAPI headers. It never touches kernel-private headers, HAL headers, or device-specific details.
2. **Profile-Driven Configuration**: Feature sets, sizes, and behaviors are selected via high-level presets (profiles) rather than arbitrary compiler flags.
3. **Backend Abstraction**: Platform-specific system calls and environment operations are accessed via a well-defined backend dispatch layer (`bh_bsys_backend_ops_t`).
4. **No Hidden State**: Avoids lazy global state, unsafe fallbacks, and non-deterministic locks, making it suitable for RT/MPU and safety-critical profiles.

## Memory operation selection

The standard copy, move, set, and compare functions start on a byte-only local
implementation. During CRT startup, `bh_libc_memops_init()` may publish exactly
one validated CPU-feature descriptor containing the feature intersection for
every CPU on which the process can run. The resolver then freezes a lib-local
GPR or x86 ERMS table. It never calls HAL and performs no feature test in the
steady-state implementations. Invalid and repeated initialization leaves the
previous safe selection unchanged.

## POSIX Is a Portability Layer

POSIX support is a source-level API and semantics layer in BharatLibC/libposix. It
is not a kernel personality and does not define a second raw syscall-number
namespace. Linux binary ABI compatibility remains a separate compatibility
personality.

The required translation path is:

```text
POSIX source API
      -> BharatLibC / libposix
      -> Bharat native API
      -> native syscall / service IPC
```

The library owns POSIX headers, `errno`, threading semantics, and the file
descriptor projection. Kernel and service interfaces remain capability-oriented
and expose Bharat-native mechanisms rather than POSIX policy.

## POSIX MVP Stages

The authoritative subset inventory is `contracts/posix_subset_v1.yaml`. Its
`implemented` list records implementation truth; its staged `functions` list is
a roadmap, not a support claim.

1. **Libc foundation:** `read`, `write`, `close`, `open`/`openat`, `lseek`,
   `fstat`, `isatty`, `clock_gettime`, `nanosleep`, and `getpid`.
2. **Memory/process:** `mmap`, `munmap`, `mprotect`, optional `brk`/`sbrk` where
   a profile requires them, and `_exit`.
3. **Threading:** thread create/join, mutexes, condition variables, and TLS.
4. **Sockets:** only after the network service ABI is stable.
