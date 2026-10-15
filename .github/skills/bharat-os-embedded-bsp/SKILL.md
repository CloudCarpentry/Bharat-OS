---
name: bharat-os-embedded-bsp
description: Apply when adding or qualifying target boards, board manifests (platform/boards/), linker templates (delivery/linker/templates/), memory layouts, MMIO exclusion zones, or embedded microcontroller bring-up (Cortex-M, RV32 MCU).
---

# Bharat-OS Embedded Board Support & Linker Framework Skill

This skill governs the declaration, qualification, and linker generation for embedded boards and microcontrollers in Bharat-OS.

## 1. Architectural Principles

- **Manifest Authority**: Every target board must have a validated declarative manifest at `platform/boards/<board_name>/board.yaml` conforming to `tools/schemas/board.schema.yaml`.
- **Zero Hardcoded Monolithic Linkers**: Never hardcode board memory addresses or section maps directly into the kernel source tree (`core/kernel/`). Linker scripts are generated from templates in `delivery/linker/templates/` via `tools/board/linker_generator.py`.
- **Strict MMIO Exclusion**: Memory regions (Flash, SRAM, CCM, DTIM) must never overlap peripheral MMIO address spaces. Automated collision detection rejects images that collide with peripherals (e.g. APB2 registers at `0x40010000`).
- **Capacity Assertions**: Linker scripts must assert at link time that `.data` + `.bss` + reserved stacks fit within physical SRAM (`ASSERT(__static_end <= __stack_limit)`).
- **Four Distinct 32-Bit Classes**:
  1. `armv7e-m`: Cortex-M microcontrollers (Thumb-2 only, NVIC, PMSAv7 MPU, Flash + SRAM layout).
  2. `armv7-a`: Cortex-A application processors (ARM mode, CP15, GIC, MMU-Lite / Full MMU).
  3. `rv32-mcu`: RISC-V Machine-mode microcontrollers (M-mode traps, CLINT timer, PMP).
  4. `rv32-sv32`: RISC-V Supervisor-mode application processors (OpenSBI, S-mode traps, Sv32 page tables).

---

## 2. Standard Workflow for Adding a Board

1. **Create Board Manifest**:
   Create `platform/boards/<board_name>/board.yaml`:
   ```yaml
   board: <board_name>
   soc: <soc_name>
   architecture: <armv7e-m | armv7-a | rv32-mcu | rv32-sv32>
   cpu: <cortex-m4 | cortex-a15 | sifive-e31 | ...>
   fpu_abi: <soft | softfp | hard>
   memory:
     flash:
       origin: 0x...
       length: ...
       executable: true
     sram:
       origin: 0x...
       length: ...
       writable: true
   mmio:
     - name: <peripheral_block>
       origin: 0x...
       length: 0x...
   runtime:
     kernel_stack: 8K
     application_stack: 4K
     heap: 0K
     mode: static
   protection:
     model: <MPU_ONLY | PMP_ONLY | MMU_LITE | MMU_FULL>
   boot:
     entry: _start
     vector_location: flash
     vector_alignment: 4
   devices:
     console: <uart_name>
     timer: <systick | clint | arch_timer>
     interrupt_controller: <nvic | plic | gicv2>
   ```

2. **Generate Linker Script**:
   ```bash
   python tools/board/linker_generator.py --board <board_name> -o build/<board_name>/linker.ld
   ```

3. **Qualify the Board**:
   Run the 10-check board qualification suite:
   ```bash
   python tools/board/validate.py --board <board_name> --check all
   ```

4. **Verify Tests**:
   ```bash
   python -m pytest quality/tests/board/
   ```

---

## 3. Mandatory Verification Checklist

Before completing any board or linker task:
- [ ] Manifest conforms to `tools/schemas/board.schema.yaml`.
- [ ] `tools/board/validate.py --board <board_name> --check all` reports all checks `[PASS]`.
- [ ] No MMIO overlap exists between executable/writable memory and hardware peripherals.
- [ ] Link-time stack assertion is present in generated script.
- [ ] `quality/tests/board/` passes cleanly.
