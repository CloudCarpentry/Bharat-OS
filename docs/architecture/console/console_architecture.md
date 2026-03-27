# Console Architecture

This document describes the design principles, layering, and responsibilities for the console and standard I/O in Bharat-OS. The architecture separates early-boot and emergency kernel mechanisms from user-facing policy and rich standard I/O management.

## 1. Core Principles

- **Mechanism in the Kernel, Policy in Userspace:** The kernel handles only the minimal, necessary mechanisms for early boot, safe panic output, and a minimal runtime fallback. All user-facing multiplexing, terminal sessions, and standard I/O (stdin/stdout/stderr) are managed by the userspace `services/system/console`.
- **Driver Integration:** The console is built *on top* of the existing driver model. It does not reinvent hardware support. The stack relies on `drivers/bus`, `drivers/devices` (class modeling), and `drivers/serial`.
- **Safe Kernel Panic:** The emergency panic path is guaranteed to be lockless, allocation-free, and polling-based. It bypasses any queues, interrupts, or scheduler states to ensure deterministic output even during severe system corruption.

## 2. Architecture Layers

### 2.1 Driver Layering (`drivers/`)
Hardware support for serial communication resides entirely within the `drivers/` subsystem. The kernel console binds to a generic device class, rather than directly to an architecture-specific UART.

1. **Bus & Device Discovery (`drivers/bus`, `drivers/devices`):** Hardware is discovered via ACPI, Device Tree, or platform-specific mechanisms.
2. **Serial Class (`drivers/class`):** Exposes a consistent `uart_driver_ops_t` interface for all serial devices.
3. **Hardware Drivers (`drivers/serial`):** Specific implementations like `ns16550`, `pl011`, or `sifive_uart` implement the interface. Advanced features (FIFOs, interrupts, DMA) are exposed here but remain optional capabilities.

### 2.2 Kernel Console Mechanisms (`kernel/src/console`)
The kernel manages three distinct console phases:

- **Early Boot Console:** Initialized immediately after basic memory is available. It binds to an early serial fallback (e.g., a simple MMIO UART or an explicitly nominated boot UART) for basic debug output before the full driver model is up.
- **Runtime Kernel Console:** Once the full device model is initialized, the console binds to the registered primary serial device. It handles standard kernel logging (`console_vlog`, `console_log`) via bounded buffers.
- **Panic-Safe Console:** Upon a kernel panic (`console_enter_panic`), the console switches to a synchronous, lockless path (`console_panic_flush_backends`). It forces output through polling-based `putc` or `write_atomic` methods, ignoring any runtime locks or asynchronous queues.

### 2.3 Userspace Console Service (`services/system/console`)
This service acts as the central broker for standard I/O across the system.

- **Responsibilities:**
  - Standard I/O (stdin/stdout/stderr) brokering for applications.
  - Session and terminal (TTY) multiplexing.
  - Interactive shell attachment.
  - Routing logs to persistent storage or diagnostic endpoints.
  - Enforcing policy on stream access.
- **Hardware Access:** It does not own the UART hardware driver directly. Instead, it relies on the kernel/driver capability mechanisms to write to the underlying hardware sink, while providing rich features (e.g., ANSI colors, UTF-8 rendering) at the user level.

## 3. Fallback Order and Selection Rules

The platform nominates the preferred console device. If the preferred device is unavailable, the fallback order is:
1. Platform-nominated primary console (e.g., specific `ns16550` or `pl011`).
2. Generic MMIO fallback (e.g., simple `uart_simple_mmio` for SBI/HTIF).
3. Memory log (`memlog_console`) as a last resort sink.

The platform is responsible for wiring the correct initialization sequence during boot to ensure the kernel console binds to the intended hardware.

## 4. Testing Strategy

- **QEMU Tests:** E2E testing must capture serial logs from QEMU to validate early boot messages, successful driver binding, and deterministic panic output.
- **Board Tests:** Hardware smoke tests must verify the correct fallback and primary console initialization on physical boards, ensuring UART output matches the expected profile.
