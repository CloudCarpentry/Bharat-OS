# ADR-017: Split BSP Global Initialization from AP Per-Core Publication

## Status

Accepted

## Context

SMP boot previously allowed secondary CPUs to run local IRQ, timer, VMM, uRPC, and scheduler initialization before the BSP had initialized the global interrupt controller, global timer source, or scheduler state.  In particular, the legacy `sched_init()` routine resets all runqueues and allocates all bootstrap scheduler objects, so an AP calling it could become the accidental authority for scheduler state belonging to other cores.

## Decision

Bharat-OS separates BSP-owned global initialization from AP-owned per-core publication:

```text
BSP: PMM -> MM global -> IRQ global -> timer global -> scheduler global -> AP launch
AP:  arch local -> cpu-local -> IRQ local -> timer local -> MM cpu online -> uRPC -> scheduler cpu online -> ONLINE
```

The scheduler contract is split into:

```c
sched_global_init(core_count);
sched_cpu_prepare(cpu_id);
sched_cpu_online(cpu_id);
sched_system_enable();
```

The memory contract is split into:

```c
mm_global_init();
mm_cpu_prepare(cpu_id);
mm_cpu_online(cpu_id);
```

The current implementation is a compatibility step: global scheduler bootstrap still reserves all bounded per-core runqueues from the BSP, and AP scheduler publication validates that the global scheduler authority already exists instead of resetting or allocating foreign runqueues.  Global VMM bootstrap remains BSP-owned, while AP memory publication initializes only local page-table/TLB glue and validates that the BSP-published kernel address-space authority is ready.

## Invariants

- The BSP is the only core allowed to run global scheduler or global VMM initialization during boot.
- APs must not reset or allocate another core's runqueue during online publication.
- APs initialize local IRQ and timer state only after the BSP has reported global IRQ and timer readiness.
- SMP timeout accounting uses the global timer after `hal_timer_init()` has completed.
- Runtime evidence reports requested, online, and failed CPU masks rather than a hard-coded core count.

## Consequences

This does not complete real SMP startup on x86_64 or RISC-V64, nor does it make the ARM64 TTBR handoff fully production-grade.  It removes the cross-core ownership inversion in the common boot sequence and creates the API boundary needed for later per-core scheduler, memory-cache, TLB-inbox, and AP failure/retry work.
