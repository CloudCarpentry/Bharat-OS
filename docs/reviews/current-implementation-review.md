# Current Implementation Review (Status + Remaining Gaps)

This review tracks the implementation state of Bharat-OS and prioritizes what remains for production readiness.

## Recently addressed

1. **POSIX typedef conflict (`size_t`)**
   - `lib/posix/include/unistd.h` now relies on `<stddef.h>` for `size_t` and centralizes POSIX scalar typedefs through freestanding `lib/posix/include/sys/types.h`.

2. **Kernel compile flag list handling**
   - Kernel CMake uses per-flag list entries in `target_compile_options` / `target_link_options`.

3. **Boot sequencing beyond HAL-only bring-up**
   - Boot flow now includes PMM/VMM init, NUMA discovery, SMP bootstrap call path, per-core URPC setup, scheduler, trap gate, timer/device framework setup.

4. **URPC API validation and typed errors**
   - Ring init/send/receive validate configuration and return explicit typed errors.

5. **Zero-copy NIC mapping path**
   - NIC mapping resolves MMIO windows from the device framework registry and performs VMM map/unmap rollback on partial failure.

6. **RISC-V Shakti BSP baseline wiring**
   - Added board-profile descriptors (`shakti-e/c/i`) and propagated OpenSBI `(hartid, fdt_ptr)` boot arguments into HAL initialization.

7. **OpenSBI payload build path**
   - Added RISC-V `kernel.payload.bin` generation and GCC toolchain/preset path for `riscv64-unknown-elf-*` workflows.

## High-priority remaining gaps

### P1 — Architecture trap and interrupt entry completeness
*(Addressed: IDT and APIC/IOAPIC initialization for x86_64; PLIC, STIE, and `stvec` routing for RISC-V; and GICv2/v3 initialization, `vbar_el1` and Generic Timer setup for ARM64 have been implemented.)*

### P1 — Scheduler/runtime depth

- Scheduler still uses static in-kernel slot arrays with no full context-save/restore assembly implementation.
- No per-core runqueue/NUMA-aware placement policy yet.

### P2 — Memory subsystem depth

- VMM remains a software mapping registry baseline and not a complete page-table manager across all architectures.

### P2 — Capability policy and MAC enforcement depth

- Capability table and delegation baseline exists, but no full policy engine/audit sink integration is in place.

## Validation checklist (living)

- `cmake --preset x86_64-elf-debug`
- `cmake --build --preset build-x86_64-kernel`
- `cmake --preset riscv64-shakti-e-gcc-debug`
- `cmake --build --preset build-riscv64-shakti-e-gcc-kernel`
- `cmake --preset tests-host`
- `cmake --build --preset build-tests`
- `ctest --preset run-tests`
- `cmake -S . -B build-tidy -DBHARAT_ENABLE_CLANG_TIDY=ON -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-elf.cmake`
- `cmake --build build-tidy --target kernel.elf`
