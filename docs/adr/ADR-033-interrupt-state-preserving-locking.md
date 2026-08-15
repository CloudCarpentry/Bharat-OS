# ADR-033: Interrupt-state-preserving kernel locking

## Status

Accepted (2026-08-13)

## Decision

Architecture HAL CPU backends expose `hal_irq_save_disable()` and
`hal_irq_restore()`. The saved value is opaque outside the implementing
architecture, is CPU-local, and must be restored on the same CPU before the
protected operation returns. Nested callers therefore preserve the interrupt
mask present at entry instead of unconditionally enabling interrupts.

Kernel code that disables interrupts temporarily must use this paired contract.
Code that also takes a spinlock may use `spin_lock_irqsave()` and
`spin_unlock_irqrestore()`. Lock order is interrupt masking before spinlock
acquisition, then spinlock release before interrupt-state restoration. Saved
state must never be placed in an IPC message, retained across migration, or
restored by another CPU.

Explicit `hal_cpu_enable_interrupts()` remains valid only for lifecycle
transitions whose contract intentionally establishes an interrupt-enabled
execution context, such as boot handoff and scheduler dispatch. It is not a
valid critical-section exit operation.

## Invariant and failure behavior

The invariant is: a function must never make interrupts more enabled than they
were on entry. Restore uses the architecture snapshot rather than inferred
software state. Invalid cross-CPU or unmatched restoration is a caller contract
violation; the snapshot is deliberately not a transferable kernel object.

## Architecture mapping

- x86_64 snapshots RFLAGS and restores the interrupt-enable bit.
- AArch64 snapshots DAIF and restores its interrupt mask.
- ARM32 snapshots CPSR and restores its interrupt mask bits.
- RISC-V snapshots the active privilege status CSR and restores its global
  interrupt-enable bit.

Host tests prove nested save/restore and IRQ-safe spinlock behavior. The five
architecture build-and-smoke matrix provides compile and runtime integration
evidence for each backend.
