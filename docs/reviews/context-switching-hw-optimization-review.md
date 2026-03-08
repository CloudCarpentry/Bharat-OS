# Context Switching Review: Kernel + Subsystem

## Scope
- Kernel scheduler context-switch path (`sched.c`) and architecture-facing selection hooks.
- Subsystem CPU allocation model (`subsys/src/*.c`).

## Findings (Gaps)
1. **L1 scheduler path claimed bitmap optimization but still performed full linear scans.**
   - Existing code comments mentioned `__builtin_clz`-style bitmask selection, but implementation still iterated all priorities.
2. **Redundant context-switch churn when scheduler re-selected the currently running thread.**
   - This inflated software switch counters and wasted preemption path work.
3. **No hardware-informed dynamic slice adaptation in runtime path.**
   - Telemetry was collected but not used to tune slice length.
4. **Subsystem CPU masks were not normalized against platform-supported core range.**
   - A subsystem could carry an all-zero or out-of-range mask until runtime use.

## Improvement Plan
1. Add and maintain a per-core active-priority bitmask in runqueue state.
2. Use highest-set-bit selection (`clz`) in L1 pick-next path for O(1) priority discovery.
3. Avoid no-op switch accounting when next == current.
4. Feed AI complexity telemetry back into bounded time-slice tuning.
5. Normalize subsystem CPU masks at creation/start so execution domains always map to valid cores.

## Implemented in this change
- Added `active_priority_mask` maintenance on enqueue/dequeue and used it in L1 scheduler selection.
- Added self-switch fast path in `sched_switch_to`.
- Added bounded dynamic time-slice shaping based on predicted complexity.
- Added subsystem CPU mask normalization helper and enforcement at subsystem creation/start.

## Next candidates (not yet implemented)
- Save/restore SIMD/FPU/vector context lazily by arch (x86 XSAVE/XRSTOR, ARM64 FP/SIMD, RISC-V V extension).
- Add explicit CPU count query in HAL (instead of fixed max assumption in multiple modules).
- Introduce per-core runqueue locking + remote wakeups/IPI balancing for true SMP concurrency.
