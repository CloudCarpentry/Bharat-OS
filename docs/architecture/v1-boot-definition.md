# v1 Boot Definition

## Purpose

This document defines a **single, testable v1 boot target** so bring-up work can be measured consistently across architectures.

## Architecture Priority (v1)

1. **x86_64 = first-class runtime path** (canonical bring-up target in QEMU).
2. **riscv64 = secondary runtime path** (must track the same milestone shape, but can lag x86_64).
3. **arm64 = compile-validation only** (no runtime success claim in v1 until explicitly promoted).

## What counts as boot success

### x86_64 boot success (runtime)

A boot is considered successful on x86_64 when **all** of the following occur in one QEMU run:

1. Kernel entry is reached.
2. Early serial console is initialized.
3. Boot banner prints architecture + build hash.
4. Physical memory map is parsed.
5. PMM is initialized.
6. Minimal VMM/kernel mappings are initialized.
7. Interrupt descriptor path is installed.
8. Timer source is initialized (PIT/APIC or equivalent).
9. Idle task is created.
10. Scheduler is entered.
11. One kernel self-test suite runs and reports pass.
12. System remains stable for **N timer ticks** (default: 1000) **or** exits QEMU with success code.

### riscv64 boot success (runtime)

A boot is considered successful on riscv64 when the same criteria are met with architecture-specific equivalents:

- Trap vector installed (`stvec`) instead of x86 IDT terminology.
- Timer initialized via SBI/timebase path.
- Physical memory map sourced from DTB/FDT.

The pass/fail bar is the same as x86_64: all milestones, self-test pass, and stability for N ticks or clean emulator success exit.

## What is compile-only vs runtime-tested

| Architecture | Compile-only requirement | Runtime-tested requirement in QEMU |
| --- | --- | --- |
| x86_64 | Must compile | **Required** for v1 success |
| riscv64 | Must compile | **Required** for v1 success (secondary/catch-up lane) |
| arm64 | **Required** for v1 | Not required in v1 |

## Required serial console evidence

A runtime-tested boot (x86_64/riscv64) must emit serial logs containing, at minimum, one line for each checkpoint:

- `BOOT: entry reached`
- `BOOT: console ready`
- `BOOT: banner arch=<arch> build=<hash>`
- `BOOT: memory map parsed`
- `BOOT: pmm initialized`
- `BOOT: vmm initialized`
- `BOOT: interrupts ready`
- `BOOT: timer ready`
- `BOOT: idle task created`
- `BOOT: scheduler entered`
- `TEST: kernel self-tests passed`
- `BOOT: stable ticks=<count>` **or** `BOOT: qemu-exit success`

Exact wording can evolve, but each semantic checkpoint must remain machine-detectable in logs.

## Phase 1 implementation order (x86_64 canonical)

Bring-up tasks should land in this sequence:

1. Early serial console
2. Boot info handoff parser
3. Physical memory map parse
4. PMM init
5. Minimal VMM/kernel mappings
6. Interrupt init
7. PIT/APIC timer (or equivalent)
8. Idle loop
9. Kernel self-tests
10. Minimal scheduler
11. Minimal endpoint IPC smoke test

This ordering is the default dependency chain for v1 x86_64 bring-up and the template for riscv64 parity.

## Phase 2 runtime proof: hello service (user space)

After Phase 1 boot stability is reached, v1 must prove the capability-mediated IPC model with one tiny user-space interaction:

1. Launch one trivial user-space `hello` task/service.
2. Grant it exactly one IPC endpoint capability.
3. Send one IPC request message from the kernel/root task side.
4. Receive one reply from the `hello` service.
5. Print success on serial console.

This is the first runtime proof point for the microkernel claim (capability + IPC behavior), not just compile/boot text output.

### Required Phase 2 serial evidence

- `P2: hello service launched`
- `P2: capability granted id=<id> rights=ipc`
- `P2: ipc request sent`
- `P2: ipc reply received`
- `P2: hello ipc smoke test passed`

Exact token spelling may change, but these semantics must stay machine-detectable.

## Scope guardrails (explicit defer list)

The following areas are **not** v1 boot-success blockers and remain deferred while Tier A/B kernel work is completed:

- GUI compositor and desktop shell
- Full network stack
- Rich filesystem feature work
- AI governor runtime integration
- Advanced personality layers
- Distributed/fabric architecture work
- arm64 runtime boot parity

For full must/should/deferred classification, see [`kernel-functionality-tiers-v1.md`](kernel-functionality-tiers-v1.md).
