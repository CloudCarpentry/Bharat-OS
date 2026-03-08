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
- Bus-aware driver/device baseline now supports explicit driver match metadata, bind lifecycle, hotplug add/remove hooks, and per-device power-state transitions.
- Built-in network defaults now include canonical scaffold entries for PCI Ethernet, USB CDC-ECM Ethernet, CAN, and virtio-net with baseline security/performance/hardware-feature flags and queue limits.

## Deferred for production

- Full APIC/IOAPIC implementation on x86 and PLIC/CLINT routing on RISC-V.
- Real periodic timer programming and interrupt ack paths.
- Driver-domain isolation and user-space driver process boundaries.
- Runtime hardware discovery from ACPI/FDT instead of static built-in tables.

## Hardware/platform subsystems still required

The v1 baseline intentionally keeps hardware support narrow. To reach serious deployment readiness across x86, ARM, RISC-V, Shakti, EV/automotive, edge devices, and datacenter targets, Bharat-OS still needs the following platform subsystems.

### 1) Board Support Package (BSP) framework

- Board descriptors with stable identifiers and capability policy defaults.
- Interrupt map description per board/SOC.
- Physical memory map description (RAM, reserved ranges, MMIO apertures).
- Firmware description ingestion:
  - Device Tree (FDT) for ARM/RISC-V/Shakti platforms,
  - ACPI for x86-class systems,
  - static board tables as fallback for bring-up.
- Flash/debug profiles (boot medium layout, debug UART/JTAG/OpenOCD presets, recovery flow hooks).

### 2) Device driver model hardening

- Bus abstraction layer so drivers bind through common probe/remove APIs.
- Driver binding and match tables (compatible string, class/subclass, vendor/device IDs).
- Power-state hooks (suspend/resume/runtime idle).
- Hotplug handling where bus semantics require it (PCIe, USB, removable media).

### 3) Bus subsystems (minimum framework per bus)

- PCI/PCIe.
- MMIO platform bus.
- USB.
- I2C.
- SPI.
- UART.
- GPIO.
- SD/MMC.
- CAN (automotive profile requirement).
- virtio (VM/QEMU profile requirement).

## Recommended implementation order

1. Land BSP descriptors + memory/interrupt maps with FDT/ACPI parsing adapters.
2. Introduce generic bus core + unified driver bind/probe/remove lifecycle.
3. Bring up UART/GPIO/I2C/SPI on platform bus first (board bring-up critical path).
4. Add PCIe + virtio for VM and server workflows.
5. Add USB/SDMMC for storage/peripheral depth.
6. Add CAN and deterministic power-state policy for automotive/EV profile.
