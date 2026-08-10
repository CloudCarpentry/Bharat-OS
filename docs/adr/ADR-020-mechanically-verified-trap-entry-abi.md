# ADR-020: Mechanically verified trap-entry ABI

## Status

Accepted

## Context

The common `trap_frame_t` layout was duplicated as numeric assembly offsets.
On RV64, XLEN-wide stores overlapped the adjacent 32-bit `type` and
`from_user` fields. ARM32 also routed IRQ entry through the SVC handler, and
x86_64 read privilege-transition-only RSP/SS slots for kernel-origin traps.
These defects make every higher-level fault and syscall decision untrustworthy.

## Decision

C layout is the sole authority for assembly-visible offsets. The target
compiler emits `trap_offsets.inc`, and the build runs the five-architecture ABI
checker before linking the kernel. Architecture entries retain their raw
fault registers in architecture-specific C-defined extensions, then common
code decodes an immutable `bh_trap_context_t` semantic view.

RISC-V writes the two fixed-width metadata fields with `sw` on RV32 and RV64.
ARM32 gives SVC, IRQ, FIQ, undefined instruction, prefetch abort, and data abort
separate vector classifications and architectural LR adjustments. ARM64 saves
ESR and FAR before C dispatch. x86_64 derives kernel RSP without reading absent
hardware fields and performs `swapgs` exactly once on each side of a user trap;
nested kernel traps do not switch GS.

## Invariants

- The raw frame is mutable owner-local state on the interrupted CPU's kernel
  stack; it is never a wire or cross-core object.
- Assembly frame accesses use generated symbols rather than duplicated C
  offsets.
- Trap metadata stores use the declared C field width.
- Origin is derived from saved architectural privilege state and fails closed.
- Fault address registers are captured at entry, before nested traps can alter
  them.
- Trap entry performs no allocation and contains no unbounded work.
- This decision does not alter syscall authorization policy or the public UAPI.

## Consequences

Changing a raw trap frame now requires regenerating offsets and passing the ABI
checker for all five mandatory architectures. The common dispatcher consumes
normalized semantic fields while existing handlers can continue to receive the
owner-local raw frame during the migration.
