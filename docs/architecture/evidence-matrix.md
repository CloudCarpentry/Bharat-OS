# Bharat-OS Claim-to-Code Evidence Matrix

This matrix provides concrete evidence for every architectural capability claimed by Bharat-OS. It connects design intent directly to the codebase and current execution maturity.

| Capability / Architectural Feature | Implementation Status | Primary Source Code Location | Test or Runtime Evidence | Known Limitations |
| --- | --- | --- | --- | --- |
| **5-Architecture Multi-Platform Support** | Implemented and runtime-verified | `core/arch/`, `core/hal/` | `tools/run_qemu_matrix.py` (Boot successful on x86_64, arm64, riscv64, arm32, riscv32) | Hardware drivers (network/storage) vary heavily by architecture. |
| **Unified 12-Byte Usercopy & Exception Table** | Implemented and test-verified | `core/arch/x86/x86_64/usercopy.c` | Host-based and unit tests. | ARM and RISC-V equivalents need validation parity with x86_64. |
| **Metadata-Driven ABI Boundaries** | Implemented and test-verified | `tools/abi/syscall_abi.py`, `interface/contracts/native_syscalls.json` | Tested during CI; ABI lock enforced. | None. |
| **32-Bit Memory Models (MMU-Lite / MPU)** | Implemented and runtime-verified | `core/kernel/` | Runtime boot validation; fail-closed MMU_FULL rejection. | Paging semantics differ from 64-bit; limits full process isolation. |
| **Hardware-adaptive capability discovery** | Implemented and runtime-verified | `core/kernel/` | Feature matrices populate successfully across architectures. | Runtime detection of deep accelerator topology is still ongoing. |
| **Capability-based security** | Partial | `core/kernel/include/capability.h` | `test_cap_*` runtime tests pass (grant, revoke, attenuation). | System-wide fail-closed enforcement across all async IPC is not yet fully locked. |
| **Per-core kernel architecture (uRPC)** | Implemented and test-verified | `core/kernel/` | SMP-VM, SMP-TLB, SMP-SCHED runtime tests pass. | Complete lockless avoidance under heavy thread-contention still undergoing hardening. |
| **Configurable Execution Profiles (GP, RT, MIX)** | Partial | `core/kernel/scheduler/` | Boot profiles loaded during initialization. | Dynamic tuning and RT deterministic strictness remain in development. |
| **Network Manager (`netmgr`)** | Partial | `core/services/netmgr` | Module initialization logged. | Production blocking receive is missing; mostly a functional scaffold. |
| **Process Manager (`process_mgr`)** | Implemented but runtime-unverified | `core/services/process_manager` | Bootstrap payload packaged by `nirmaan`. | Complete, dynamic real ELF execution loading from filesystem is missing. |
| **Virtual Memory Manager (`vm_mgr`)** | Implemented but runtime-unverified | `core/services/vm_manager` | Boot-time memory reservation and paging works. | On-demand page-pool orchestration and swap are scaffolded. |
| **Accelerator Capability & Dispatch** | Partial | `core/lib/runtime/backend_dispatch` | Basic backend routing and capability logic exists. | Production device queue realization and end-to-end service mediation missing. |
| **Heterogeneous Compute Control Plane** | Partial | `core/services/device/accelmgr` | Service is declared during boot. | Real event loop and truthful hardware backend execution are missing. |
| **Manifest-driven service initialization** | Implemented and runtime-verified | `core/services/core/init` | Init discovers and reserves payloads (namesvc, process_manager). | Subsequent daemon dependencies and full process lifecycle fail during complex boot chains. |

*Status Legend:*
* **Implemented and runtime-verified:** Works end-to-end on target or in QEMU with observable runtime state.
* **Implemented and test-verified:** Works in host unit tests or focused subsystem tests.
* **Implemented but runtime-unverified:** Code exists and compiles into target, but full end-to-end validation across the system boundary isn't currently proven.
* **Partial:** Functional pieces exist, but significant components or security gates are missing.
* **Planned:** Architectural intent only.
