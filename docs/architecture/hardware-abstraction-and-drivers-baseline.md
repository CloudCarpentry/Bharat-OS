# Hardware Abstraction and Driver Baseline (v1)

This document captures the current HAL and driver-framework baseline in kernel space.

## Implemented baseline

- Interrupt path scaffolding:
  - architecture hook for interrupt-controller initialization (`hal_interrupt_controller_init`),
  - IRQ route API (`hal_interrupt_route`),
  - common IRQ handler registration/dispatch table (`hal_interrupt_register`, `hal_interrupt_dispatch`).
- Timer path scaffolding:
  - architecture hook for timer source setup (`hal_timer_source_init`),
  - common monotonic tick accounting (`hal_timer_tick`, `hal_timer_monotonic_ticks`).
- Generic device framework:
  - driver registration API (`device_register_driver`),
  - MMIO window registry and lookup (`device_register_mmio_window`, `device_lookup_mmio_window`),
  - generic IRQ fanout (`device_dispatch_irq`).
- Built-in baseline drivers/windows:
  - UART/SPI/I2C/SDMMC/Ethernet driver records,
  - Ethernet RX/TX MMIO window registration for NIC IDs.
- Zero-copy NIC integration now resolves MMIO windows through device framework lookups instead of local hardcoded tables.

## Deferred for production

- Full APIC/IOAPIC implementation on x86 and PLIC/CLINT routing on RISC-V.
- Real periodic timer programming and interrupt ack paths.
- Driver-domain isolation and user-space driver process boundaries.
- Runtime hardware discovery from ACPI/FDT instead of static built-in tables.
