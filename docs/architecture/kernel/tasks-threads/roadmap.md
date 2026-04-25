# Tasks & Threads Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `core/kernel/src/sched/` and `core/kernel/src/trap/`:
- ✅ **Basic Thread Control Block (`bh_thread_t`)**: Implemented.
- ✅ **Context Switching**: Implemented core trap frame saving/restoring (`trap_frame_t`).
- ✅ **Syscall Dispatch**: Basic `syscall_dispatch` and `trap_handle` implemented.
- ✅ **Basic States**: Ready, Running, Blocked, Exited.
- 🟡 **Distributed States**: Adding `REMOTE_HANDOFF_PENDING` to track safe cross-core handoff transitions before full task migration.
- 🟡 **Task/Process Abstraction**: Partially implemented; strong separation of ASpace and CSpace ownership is stabilizing.
- 🟡 **FPU/SIMD Context Saving**: Lazy saving is planned but architecture-specific implementations are ongoing.

## Near-Term Goals (Next 3-6 Months)
1. **Full Multikernel Task Isolation**: Ensure strict ASpace/CSpace enforcement without legacy monolithic bypasses.
2. **Architecture-Specific Entry Stubs**: Finalize ASM entry points for x86_64 IDT and RISC-V `stvec`.
3. **Futex-like Fast Mutexes**: Implement user-space fast paths for synchronization to minimize syscall overhead.
4. **TLS Support**: Standardize thread-local storage ABIs across all supported architectures (x86_64, arm64, riscv).
5. **Thread Handoff Ownership Rules**: Formalize rules around thread object residency vs capability ownership when moving between core runqueues.

## Long-Term Vision (1+ Years)
1. **Dynamic Task Migration**: Safely migrate tasks between multikernel cores (in conjunction with the AI Scheduler).
2. **Deterministic Real-Time Threads**: Guarantee bounded context-switch latencies for RT-profile builds.
3. **Hardware-Assisted Isolation**: Leverage virtualization extensions (e.g., VT-x, ARM EL2) for deeper task sandboxing where applicable.