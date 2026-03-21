# Bharat-OS Console Subsystem Architecture

The Bharat-OS console subsystem is a production-baseline, capability-driven stack designed for robust early boot, runtime formatting, and hardened panic modes.

## Layering

1.  **Console Core** (`kernel/include/console/`, `kernel/src/console/`):
    Manages global state, phase transitions (`early`, `runtime`, `panic`), formatting bounds (`console_vlog`), the central ring buffer (`console_buffer`), and routing to active backends.
2.  **Console Backends** (`memlog_console.c`, `serial_console.c`, `framebuffer_console.c`):
    Expose hardware or virtual sink behavior through a unified capability contract (`console_backend_ops_t`). The `memlog` backend acts as a reliable storage sink tightly coupled with the core ring buffer. The `serial` and `framebuffer` backends dispatch to specific hardware drivers.
3.  **Hardware Drivers / Renderers** (`drivers/uart_*.c`, `console_render_fb.c`):
    Implement register-level manipulation (e.g., PL011, NS16550) and specific text/pixel rendering (e.g., framebuffer text wraps, scrolls) keeping quirks out of generic logic.
4.  **Discovery & Policy** (`console_discovery.c`, `console_policy.c`, `arch/*/console/arch_console_discovery.c`):
    Platform-specific stubs discover board UART/display hardware statically or via DTB/ACPI and report unified descriptors. The policy module selects the best early, runtime, and panic sinks based on priority, caps, and phase suitability without hardcoding rules into the main core logic.

## Type Discipline

*   `uintptr_t` is used for hardware mmio base addresses and framebuffer bases.
*   `console_caps_t` provides 32-bit flags defining backend behavior (`CON_CAP_WRITE_POLL`, `CON_CAP_PANIC_SAFE`, `CON_CAP_VISIBLE_SINK`, etc.).
*   `console_seq_t` and `console_index_t` are strictly 32-bit to maintain compact sizes and avoid unnecessary 64-bit atomic overhead where 32-bit suffices.
*   The `console_record_t` is a fixed-size compact struct allowing deterministic ring buffering without heap allocation or variable length memory copies during panic phases.

## Panic Mode

During a kernel panic, standard locking is bypassed, and only backends advertising `CON_CAP_PANIC_SAFE` or flagged `panic_ok` receive logs. The `write_atomic` function on backend ops takes precedence over standard `write` to bypass interrupts and rely on safe polling routines.

## Userspace Integration

A skeleton userspace daemon (`services/console/main.c`) defines a stable 32-bit IPC message ABI (`console_msg_hdr_t`) for handling console interactions across the boundary.
