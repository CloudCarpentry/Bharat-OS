# Large Module Refactoring Plan

This document identifies files that suffer from extreme size and bloat, and provides the architectural roadmap for decomposing them.

## Target 1: `core/kernel/src/capability.c` (Monolithic Capability Registry)
- **Problem**: This file contained ~1,400 lines of highly coupled code, mixing memory-management for capability nodes (c-spaces) with IPC message dispatch, validation logic, and lookup paths.
- **Solution**: The file has been successfully split into two modules:
  - `core/kernel/src/cap_cspace.c`: Responsible for CNode allocation, `capability_table_t` management, capability slot granting, copying, and CSpace lookup behaviors.
  - `core/kernel/src/cap_dispatch.c`: Responsible for unpacking uRPC packets representing cross-core capability delegation, revocation, and message passing mechanisms.
- **Outcome**: The original `capability.c` file acts as the primary runtime entry point and validation module, while delegating state management and cross-core message parsing to their respective domains.

## Future Targets
- **Scheduler**: `core/kernel/src/sched/sched.c` and `sched_core.c` have grown excessively large and need runqueue mechanics separated from policy evaluation.
- **PMM**: `core/kernel/src/mm/pmm/pmm.c` manages initialization, buddy allocation, and page fault logic all at once.
