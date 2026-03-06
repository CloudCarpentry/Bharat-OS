# Bharat-OS

A scalable, POSIX-compatible microkernel operating system built from scratch, designed to support multiple hardware architectures (x86, RISC-V, Shakti, GPUs, NPUs) and host subsystems (Linux, Windows, BSD, Darwin).

## The Vision & Scope

Bharat-OS avoids the trap of "trying to be everything." We explicitly separate the fundamental kernel architecture from experimental research and deferred subsystems.

### The Fundamental Core (v1 Spine)

- **Verification-First Microkernel**: A tiny, auditable foundation (inspired by seL4) providing only thread scheduling, capabilities, and basic IPC. Immune to memory-unsafe sprawl.
- **Multikernel Architecture**: Lockless inter-core messaging (inspired by Barrelfish). Treats high-core-count CPUs as distributed systems to prevent shared-memory bus contention.
- **Permissive Toolchain**: Built exclusively with LLVM/Clang and musl libc to guarantee an MIT/Apache licensing stance. Avoids GNU/GPL components natively.
- **Phased Architecture Support**: Initial bring-up targets: x86_64 and RISC-V under QEMU. ARM64 is planned once the common boot, paging, and interrupt abstractions stabilize.

## Two Official Product Lines

To avoid contradictory performance goals, the identical v1 microkernel is deployed as two distinct system profiles:

1. **Bharat-RT (Real-Time)**: Bounded latency, predictable scheduling, and static capability allocation. Optimized strictly for aerospace, defense grids, and embedded IoT.
2. **Bharat-Cloud (Throughput)**: Elasticity, rich virtualization, and massive I/O pipelines. Optimized for data centers, POSIX server virtualization, and multi-GPU CXL fabrics.

### Supported Personalities & Modes (Bharat-Cloud)

- **Cloud-Native Unikernels**: Optional single-address-space/library-OS deployment mode for statically linked trusted workloads.

### Research Horizons & Future Expansion (Deferred)

- **Linux Subsystem**: Compatibility subsystems are future research modules, beginning with a Linux syscall personality.
- **CXL & Accelerator Fabrics**: Future-facing native awareness of CXL 3.x memory pooling.
- **AI-Aware Tuning via User-Space**: The kernel exports telemetry, tunables, and policy hooks, allowing user-space daemons to run ML heuristics and push bounded policy updates, preserving kernel determinism.
- **Hardware Enclaves**: SGX/TrustZone cryptographic abstraction.
- **Advanced File Systems**: ZFS-inspired native formats with inline deduplication and CoW.

## Project Structure

- `kernel/`: Core microkernel source (hardware abstraction, IPC, capability model, thread scheduling).
- `lib/`: Standard C library (e.g., musl or FreeBSD libc) and common APIs.
- `drivers/`: Hardware device drivers (run in isolated user-space).
- `services/`: Future subsystem, accelerator, and POSIX native endpoints.
- `tools/`: Build tools, scripts, and host utilities.
- `subsys/`: Subsystem compatibility layers.
- `profiles/`: Build policies splitting the core into `rt/` (Real-Time) and `cloud/` products.
- `docs/`: Technical documentation, architectural specs, and ADRs.

## Current Implementation Status

We are currently in the **Architectural Scaffolding Phase**. The following core subsystem APIs and C-headers form the **v1 bootable core**:

- **Kernel Core & HW Abstraction (`kernel/`)**:
  - `hal_cpu.c` (x86_64, RISC-V) - CPU architecture interrupt handling and HAL layer.
  - `boot/x86_64/multiboot2.h` & `boot/riscv/sbi.h` - Kernel entry interfaces for UEFI and OpenSBI.
  - `mm/pmm.c` & `mm/vmm.c` - Physical Bitmap Allocator and Demand Paging Virtual Memory managers.
  - `sched.h` - Process and Thread scheduling structures.
- **Advanced Paradigms**:
  - `multikernel.h` - Lockless URPC rings for Inter-Core IPC.
  - `unikernel.h` - Direct-to-metal Library OS compilation flags.
  - `capability_model.h` - Capability tokens for the verification scope bounding.
- **Profiles (`profiles/`)**:
  - `rt/policy.h` - Hard real-time scheduler constraints scaffolding.
  - `cloud/policy.h` - High-throughput environment scaffolding.
- **Emulation Tooling (`tools/`)**:
  - `generate_vm.py` - Scaffolds QEMU execution VMs for all supported hardware architectures.

> _Initial API sketches exist for future storage (BFS/VFS), clustering (CXL/RDMA), accelerators (GPU/NPU in `services/accel/`), and hardware enclaves, but they are explicitly restricted from the v1 bootable core namespace._

## Implementation Roadmap

### Phase 1: The Core Spine (Current Focus)

- Construct the Multiboot2 (for x86) and SBI (for RISC-V) entry stubs.
- Build the physical memory allocator and basic demand paging.
- Establish the Core-to-Core lockless URPC messaging mechanisms.

### Phase 2: Personalities & Standard APIs

- Flesh out standard POSIX abstractions.
- Prototype the Unikernel (Library OS) static-linking compiler flags.
- Establish the Linux Syscall translation tables.

### Phase 3+: Deferred & Research Horizons

- Porting complex storage (BFS/VFS).
- ML-tuned real-time schedulers in user-space via bounded kernel policy hooks.
- CXL device enumeration and distributed node tracking.

## Building & Emulating

Please see [BUILD.md](./BUILD.md) for complete, cross-platform instructions on how to install the required LLVM/Clang and CMake toolchains across **Linux, macOS, and Windows**, compile Bharat-OS, and run the automated QEMU emulation scripts.
