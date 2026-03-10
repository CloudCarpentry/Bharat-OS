# Subsystems Architecture Review & Gap Analysis

## 1. Overview & Goal

This document reviews the Bharat-OS subsystem structure (GUI, Console, Framebuffer, File System, Network) against multi-architecture goals (x86_64, ARM32/ARM64, RISC-V 32/64, SHAKTI, QEMU) and multiple profiles (Tiny Embedded, RT, Edge, Desktop/Server). The core principle is keeping the Bharat-OS microkernel small and capability-driven, pushing policy and complex stacks into user-space services, while maintaining a unified abstraction layer across diverse hardware.

## 2. Current Subsystem Inventory & Gap Analysis

### 2.1 Console Subsystem
*   **Inventory:**
    *   **HAL Layer:** Basic early-boot serial routines exist in `kernel/include/hal/hal.h` (`hal_serial_init`, `hal_serial_write_char`, etc.).
    *   **x86_64:** `kernel/include/hal/serial.h` has a basic COM1 `outb`/`inb` polling implementation.
    *   **RISC-V/SHAKTI:** Basic UART base addresses exist in `shakti_bsp.c`.
*   **Gaps & Problems:**
    *   No generic terminal/console abstraction layer.
    *   No multiplexing (e.g., routing `printf` from different kernel/user domains).
    *   No split between early-boot polling console and interrupt-driven runtime console.
    *   No user-space `services/console` daemon to handle TTY/PTY emulation.

### 2.2 GUI / Framebuffer / Display Path
*   **Inventory:**
    *   **UI Layer:** `ui/fbui/core/` contains `fb_render.c` with a basic software 2D renderer (fill rect, draw line, blit) assuming a flat 32-bpp framebuffer (`bharat_display_device_t`).
*   **Gaps & Problems:**
    *   **Coupling:** `fb_render.c` references `bharat_display_device_t`, but this device struct is not cleanly defined in the core kernel's device framework (`kernel/include/device.h`).
    *   No HAL/Driver mapping for setting up framebuffers (e.g., parsing UEFI GOP on x86, or configuring DRM/KMS on ARM/RISC-V).
    *   No text-mode console rendering over the framebuffer.

### 2.3 File System
*   **Inventory:**
    *   **Service Layer:** `services/file_system/main.c` exists but is just an empty stub with `TODO`s for POSIX semantics and block devices.
*   **Gaps & Problems:**
    *   No core VFS (Virtual File System) interface (`mount`, `open`, `read`, `write`, `ioctl`).
    *   No initial RAM-backed filesystem (ramfs/tmpfs) for early boot or diskless profiles.
    *   No IPC bindings connecting the VFS service to capability-based core URPC.

### 2.4 Network Module
*   **Inventory:**
    *   **Device Layer:** `kernel/include/device.h` defines `DEVICE_CLASS_ETHERNET` and `DEVICE_CLASS_WIFI`.
    *   **Driver:** `drivers/net/ptp_clock.c` exists (likely stubbed).
    *   **IO Layer:** `kernel/include/io_subsys.h` outlines a high-throughput zero-copy ring (`io_ring_t`, `zero_copy_nic_ring_t`) inspired by io_uring/DPDK.
*   **Gaps & Problems:**
    *   No generic network device abstraction (`netdev`).
    *   No packet buffer abstraction (`mbuf`/`skb`).
    *   No user-space network service (`services/net/`) to handle protocols (ARP, IP, UDP, TCP).

## 3. Subsystem Layering & Separation

To support multi-arch and multi-profile scaling without bloating the microkernel:

*   **Core Kernel (Ring-0):**
    *   **Only mechanisms, no policies.**
    *   `hal_serial_*` for early kernel panics.
    *   Memory mapping (VMM) and MMIO window granting for device drivers.
    *   URPC/IPC channels.
*   **Hardware Abstraction Layer (HAL) / BSP:**
    *   Architecture-specific initialization (e.g., GIC/PLIC).
    *   **SHAKTI SDK Note:** The HAL/BSP must parse the FDT (Device Tree) or use static definitions to expose the UART, PLIC, CLINT, and RAM. SHAKTI-specific memory maps and boot sequences remain entirely in `kernel/src/hal/riscv/shakti_bsp.c`.
*   **Drivers (User-Space / Bounded Kernel):**
    *   Actual drivers for UART, NICs, and GPUs. These interact with hardware via capability-gated MMIO windows and interrupts.
*   **Services (User-Space Daemons):**
    *   `services/console`: Owns the active UART driver capability. Provides TTY streams to other apps via URPC.
    *   `services/file_system`: Implements VFS, ramfs, and later block-backed FS. Exposes file handles via capabilities.
    *   `services/net`: Owns the NIC driver capability via `io_setup_zero_copy_nic_ring`. Implements the network stack (L2-L4).
*   **Personality/Profile Layer:**
    *   Translates POSIX `read(fd)` syscalls into IPC messages to `services/file_system` or `services/console`.

## 4. SHAKTI SDK Specific Expectations

The SHAKTI SDK provides a specific toolchain and boot environment (often OpenSBI-based or direct M-mode boot).
*   **Boot/BSP:** The FDT parser in `shakti_bsp.c` is the right approach. It must correctly extract memory boundaries and device MMIO ranges before the core kernel VMM initializes.
*   **Console/UART:** SHAKTI typically uses a simple 16550-compatible UART or a custom AXI UART. The early boot console should poll this address. Later, a user-space driver will map the UART MMIO window and bind to its PLIC interrupt.
*   **Timers/Interrupts:** Standard RISC-V CLINT (Core Local Interruptor) for timers and PLIC (Platform-Level Interrupt Controller) for external interrupts.
*   **Framebuffer:** SHAKTI boards (like Artix7 FPGAs) may lack a standard GPU. Framebuffer support will rely either on an IP block (e.g., a simple AXI video out) or be headless. The architecture must gracefully fallback to headless/serial console if no framebuffer device is registered.

## 5. Phased Implementation Roadmap

### Phase 0: Must-Have Bring-Up (Current Focus)
*   **Goal:** Boot to a stable state, parse hardware layout, and output logs.
*   **Tasks:**
    *   Solidify generic early serial console in HAL.
    *   Define core structural headers (`console.h`, `vfs.h`, `netdev.h`, `display.h`).
    *   Ensure capability tables can grant MMIO windows to drivers.

### Phase 1: Minimal Usable Services (Next Step)
*   **Goal:** Basic interaction without complex protocols.
*   **Tasks:**
    *   **Console:** Implement a simple `services/console` that reads from a serial driver and multiplexes log output.
    *   **File System:** Implement a basic in-memory `ramfs` in `services/file_system` with open/read/write semantics over IPC.
    *   **Framebuffer:** Connect the existing `fbui` to a dummy or predefined VRAM region. Add a text-rendering mode to `fbui` so the console can output to the screen.
    *   **Network:** Define the `netdev` interface and packet buffer structs. No protocol stack yet.

### Phase 2: Richer Cross-Architecture Support
*   **Goal:** Support real hardware drivers across architectures.
*   **Tasks:**
    *   Implement real UART interrupts (instead of polling).
    *   Map Block Device drivers to the VFS.
    *   Implement an L2 packet switch and basic ARP/UDP in `services/net`.
    *   Implement driver stubs for QEMU VirtIO (net, blk, gpu) for easy multi-arch testing.

### Phase 3: Advanced GUI & Personality Separation
*   **Goal:** Full POSIX compatibility and desktop-like experience.
*   **Tasks:**
    *   Connect Linux/POSIX personality syscalls (`open`, `socket`, `printf`) to the respective microkernel services.
    *   Implement a full GUI compositor over the `fbui`.
    *   Full TCP/IP stack.
