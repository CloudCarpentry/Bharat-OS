# Thread Local Storage (TLS)

## Overview
In multithreaded environments, threads within the same task share an Address Space (ASpace). Thread-Local Storage (TLS) provides a mechanism for each thread to have its own private instance of a variable. This is critical for `errno`, `pthread_self()`, and per-thread memory allocators.

## The Architecture
Bharat-OS provides TLS via hardware registers. The kernel maintains a TLS pointer in the thread's `TCB` (Thread Control Block) and loads it into the appropriate hardware register during a context switch.

### Architecture Specific Implementations

#### x86_64
- **Mechanism:** Segment registers `fs` or `gs`.
- **Context Switch:** The kernel uses `wrfsbase` or `MSR_FS_BASE` to set the base address of the `fs` segment to point to the thread's TLS block.
- **Access:** User-space accesses TLS variables via `fs:offset`. The `gs` register is typically reserved for the kernel's per-CPU data (`cpu_local_t`).

#### ARM64 (AArch64)
- **Mechanism:** The `tpidr_el0` (Thread Pointer ID Register, EL0).
- **Context Switch:** The kernel writes the thread's TLS block address to `tpidr_el0` using `msr tpidr_el0, x0`.
- **Access:** User-space reads `tpidr_el0` using `mrs x0, tpidr_el0` and then uses it as a base pointer to access variables.

#### RISC-V
- **Mechanism:** The `tp` (Thread Pointer) register (`x4`).
- **Context Switch:** The kernel saves and restores the `tp` register as part of the `trap_frame_t`.
- **Access:** User-space accesses TLS variables as offsets from the `tp` register.

## TLS Block Layout
The exact layout of the TLS block is defined by the compiler and linker (e.g., ELF `.tdata` and `.tbss` sections). The user-space runtime (libc) allocates the TLS block when a thread is created and passes the address to the kernel via a system call (e.g., `sys_set_tls`). The kernel stores this pointer in the `TCB` and loads it into the hardware register during context switches.