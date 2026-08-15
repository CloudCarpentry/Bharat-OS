---
title: Ubuntu Userland Support Roadmap
status: draft
owner: Architecture Team
last_updated: 2026-05-15
tags:
  - architecture
  - personalities
  - linux
  - ubuntu
see_also:
  - linux_personality_architecture.md
  - linux_personality_syscall_plan.md
version: 1.0
---
# Analysis Report & Roadmap: Ubuntu Userland Support

## 1. Executive Summary

The current Bharat-OS Linux Compatibility Personality provides a foundational POSIX-like user space, targeting CLI tools, headless server workloads, and static/musl-based environments. It explicitly avoids complex POSIX semantics such as a full `fork()` with deep copy-on-write (COW) assumptions and limits the scope of signals and virtual filesystems.

To support a full **Ubuntu userland** (including `apt`, `dpkg`, complex `glibc`-based daemons, and desktop/mobile environments), we must extend the Linux Personality subsystem significantly. This document provides a gap analysis and a phased execution roadmap to achieve full Ubuntu binary compatibility while maintaining the core architectural principle: **no translation tax on hot paths**.

## 2. Analysis Report: Gap Analysis

### 2.1 Current State of Linux Personality
- **Libc Support:** Optimized for custom libc or `musl`.
- **Process Model:** `clone`-based threading, but **no full `fork()`**. `posix_spawn` is preferred.
- **Syscalls:** Tier-1 hot paths (`mmap`, `futex`, `epoll`, `read`/`write`) are direct-mapped.
- **VFS & Filesystem:** Minimal VFS bridge. Fake global filesystem hierarchy backed by a capability.
- **Networking:** Baseline IPv4/IPv6 TCP/UDP mapping to the Native network service.

### 2.2 Ubuntu Userland Requirements

To boot and run a functional Ubuntu rootfs (e.g., using `apt` to install packages), the following capabilities are strictly required:

1. **`glibc` and Dynamic Linking:**
   - Ubuntu relies heavily on `glibc`, which demands strict adherence to POSIX semantics, advanced Thread Local Storage (TLS), complex memory allocator behaviors (`sbrk`/`brk`/`mmap` interplay), and `nsswitch` functionality.
   - We need a robust dynamic ELF loader (`ld-linux-x86-64.so.2`, etc.) compatible with the Bharat-OS personality layer.

2. **Process Management (`fork` and `execve`):**
   - Package manager scripts (shell scripts) and `dpkg` rely heavily on `fork()` + `execve()` patterns.
   - Bharat-OS must introduce a performant COW mechanism in the Virtual Memory Manager (VMM) to support `fork()` or deeply optimize/trap shell script execution into `posix_spawn()` equivalents.

3. **Rich Virtual Filesystem (`/proc`, `/sys`, `devpts`):**
   - Tools like `ps`, `top`, and `dpkg` require `/proc` for PID metadata and `/sys` for hardware capabilities.
   - Interactive CLI and apt operations require `pty` (pseudo-terminal) support mapped to the core console architecture.
   - `chroot` and basic mount namespace support (or convincing emulation) is required for package installations.

4. **Unix Domain Sockets (`AF_UNIX`):**
   - Crucial for D-Bus, systemd (even if stubbed), X11/Wayland, and general daemon IPC.
   - Must be mapped to Bharat-OS fast-path IPC/URPC mechanisms.

5. **Advanced Signals and User-space Exceptions:**
   - Full POSIX signal delivery semantics for job control (`SIGSTOP`, `SIGCONT`) and error handling.

### 2.3 UI / Desktop & Mobile Targets
- Requires integration of DRM/KMS abstractions for Wayland/XWayland or direct integration with the Bharat-OS `UI Surface Buffer Direct Scanout` model.
- Input subsystem (evdev) mappings.

---

## 3. Execution Roadmap

To achieve full Ubuntu userland capability, we will execute in four distinct phases.

### Phase 1: Core Kernel and Syscall Foundation
**Goal:** Run complex `glibc` binaries and shell scripts.
- **1.1 Dynamic Loader Support:** Fully implement `auxv` injection and TLS setup required by `glibc`'s `ld.so`.
- **1.2 Memory Management:** Extend the VMM to support Copy-On-Write (COW) mappings efficiently to enable full `fork()` semantics.
- **1.3 Process Lifecycle:** Implement complete `fork()`, `vfork()`, and `execve()` handlers. Ensure wait/waitpid semantics accurately reflect child states.
- **1.4 Signal Semantics:** Expand signal delivery to include job control, synchronous traps (`SIGSEGV`, `SIGILL`), and proper `ucontext` setup for signal handlers.

### Phase 2: VFS Enrichment & Emulation
**Goal:** Satisfy system state queries and interactive terminal requirements.
- **2.1 `procfs` Stubbing:** Implement a dynamic `/proc` bridge. Generate `/proc/self`, `/proc/[pid]/stat`, `/proc/meminfo`, and `/proc/cpuinfo` from native Bharat-OS state.
- **2.2 `sysfs` Stubbing:** Provide `/sys/class/net` and baseline hardware info.
- **2.3 PTY / TTY Subsystem:** Implement the `devpts` filesystem and `ioctl` sets for pseudo-terminals to allow interactive shells (bash/zsh) and SSH sessions.
- **2.4 File Locks:** Implement `fcntl` POSIX record locks and `flock`, which `dpkg`/`apt` require for database locking.

### Phase 3: Package Manager Bootstrapping
**Goal:** Run `apt-get update` and `apt-get install` successfully.
- **3.1 Chroot & Namespaces:** Provide `chroot()` syscall support backed by the capability-based VFS. Stub basic mount namespaces if required by post-install scripts.
- **3.2 Init / Systemd Bridging:** `dpkg` post-install scripts often invoke `systemctl`. Provide a wrapper/stub systemd implementation that bridges Linux daemon activation to the native Bharat-OS `init` service architecture (`core/services/init`).
- **3.3 Unix Domain Sockets:** Map `AF_UNIX` sockets to the local Native IPC loopback for D-Bus and system daemons.
- **3.4 Networking Completeness:** Ensure `getaddrinfo()` (via NSS) and `AF_INET`/`AF_INET6` sockets map perfectly to the native Netstack without performance degradation.

### Phase 4: Desktop and Mobile Integration (VM/Desktop/Mobile)
**Goal:** Launch an X11/Wayland environment or native Ubuntu Mobile applications.
- **4.1 Display & DRM Bridging:** Map Linux DRM/KMS `ioctls` to the Bharat-OS Display Lease and Framebuffer Contract (`docs/architecture/display/display-lease-and-framebuffer-contract.md`).
- **4.2 Input Routing:** Expose native input events as `/dev/input/eventX` (evdev) nodes.
- **4.3 Audio (ALSA):** Map ALSA `ioctls` and memory-mapped rings to the native audio subsystem.
- **4.4 Hardware Acceleration:** Route GPU compute/render contexts via the accelerator capability model, ensuring zero-tax memory mapping for user-space graphics drivers (Mesa).

## 4. Architectural Constraints
- **Zero Translation Tax:** Hot paths (`mmap`, `epoll`, sockets) must remain translation-free. `AF_UNIX` must map directly to Native IPC endpoints.
- **Security Boundary:** The Ubuntu rootfs will be constrained within a single capability sandbox. Escaping to the native Bharat-OS host requires explicit IPC capability grants.
