# Generic Driver Layer (Architecture + Board)

This folder contains reusable driver building blocks selected by the CMake build profile.

## Driver groups

- **Architecture generic drivers**
  - `arch_irq_timer.c`: baseline IRQ/timer bring-up hook for supported architecture families (`X86`, `ARM`, `RISCV`).
- **Board generic drivers**
  - `board_fabric.c`: board transport/fabric shim for virtual and Shakti-style boards (`qemu*`, `*virt*`, `*shakti*`).

## Build-time selection

`drivers/CMakeLists.txt` performs two-stage selection:

1. Enable/disable generic set with `BHARAT_ENABLE_DRIVER_GENERIC`.
2. Select source files from `BHARAT_ARCH_FAMILY` and `BHARAT_TARGET_BOARD`.

This keeps a single production path while preserving strict board/architecture scoping.
