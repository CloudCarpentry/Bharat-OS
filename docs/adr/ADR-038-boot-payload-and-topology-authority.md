---
title: Boot payload reservation and topology authority
status: Accepted
owner: Kernel and Services Working Group
last_updated: 2026-10-10
---

# ADR-038: Boot payload reservation and topology authority

Early kernel metadata allocation must preserve every normalized boot-module
reservation until the userspace loader consumes it. Checking only an
allocation's endpoints missed modules entirely inside that allocation; aligning
after checking could also move the allocation into a module. The x86 STATIC
bootstrap reproduced corrupted ELF magic before supervisor entry.

The BSP's existing reservation list remains the memory authority. The early
allocator aligns first, rejects arithmetic overflow, checks overlap of the
entire half-open allocation span, and retries beyond overlapping reservations.
The added query is internal to PMM; it introduces no user ABI or new owner.
Host tests cover interior reservations, subsequent reservations, alignment,
exclusive boundaries and overflow. No allocator reclamation policy changes.

SMP bootstrap consumes the same normalized HAL CPU topology used for scheduler
partitioning. An absent firmware inventory confirms only the executing BSP;
it must not create AP launch requests from a board policy's maximum alone.
Known AP launch failures retain the existing strict RT failure policy. This
corrects authority consumption; it does not implement x86 INIT/SIPI, change the
scheduler or qualify undiscovered/multiple CPUs.

The STATIC root keeps its own startup entry while using the canonical userspace
ELF configuration (`NO_CRT0`) and static linking. On x86, this preserves the
existing user/kernel virtual-address separation. Kernel mechanisms load the
root; userspace owns its runtime validation and lifecycle report. MPU hardware
capability checks and service readiness/dependency checks remain fail closed.
