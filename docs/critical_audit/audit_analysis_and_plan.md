# Audit Analysis: Capability Lifecycle and System Semantics

## 1. Overview
The current developer branch (head `1d4ddd4364760ec6bd3626f569f0da8191f1ee60`) shows substantial improvement in Arch/HAL physical separation and dependency surfaces. The old arch/hal overlay has been removed, and hal_common has a cleaner dependency surface. However, the primary focus needs to shift from folder organization to **lifecycle correctness, silent fallback handling, capability semantics, and file-level hygiene**.

## 2. Current Assessment
| Area | Assessment |
| --- | --- |
| Arch/HAL physical separation | Much better — ~8/10 |
| HAL contract design | Good foundation — ~7.5/10 |
| CPU feature lifecycle | Needs hardening — ~5.5/10 |
| Memops architecture | Good concept, weak failure semantics — ~7/10 |
| HMEM/DMA coherency | Potentially unsafe — ~4.5/10 |
| BharatLibC HW dispatch | Good prototype, not production-ready — ~6/10 |
| File-level code hygiene | Still mixed — ~6/10 |
| CI/invariant enforcement | Good structure, several major holes — ~6.5/10 |

## 3. Key Issues Identified

### P0 Issues (Critical Priority)
* **CPU Capability Finalization (`core/arch/common/cpu_caps_state.c`)**: Currently lacks transactional behavior. It ignores important return values from hardware-adaptive initialization, meaning failures silently degrade into scalar behavior. If memops freeze fails after CPU features are frozen, there is no coherent rollback, leaving the system inconsistent. Hardcodes BSP = CPU 0. Violates architectural layering by using generic kernel functions (e.g., `memset` forward declaration). Fails to check if CPU caps were actually published (`g_cpu_caps_present`).
* **Multiple CPU-Count Authorities**: Disparate definitions for max CPUs (`MAX_CPUS = 32U`, `HAL_CPU_FEATURE_MAX_CPUS = 256u`, `HAL_MEMOPS_MAX_CPUS = 256u`).
* **False Success in `hal_hmem_sync_range()`**: Returns `HAL_HMEM_OK` when no backend is available using only a `SEQ_CST` fence. This is completely insufficient for cache/DMA coherency on non-coherent devices (ARM/RISC-V). Should fail closed (`K_ERR_UNSUPPORTED`) unless globally coherent. Installation atomic checks aren't robust CAS.
* **BharatLibC CMake Modification**: `core/lib/bharatlibc/CMakeLists.txt` writes to `src/crt/common/crt_stub.c` during build (`file(WRITE)`), dynamically mutating the checked-in source tree.
* **Raw vs. Usable CPU Feature Semantics**: Some x86 features are marked usable almost immediately after CPUID without ensuring OS context state ownership, kernel policy, or required control registers are enabled.

### P1 Issues (High Priority)
* **CPU Feature Normalization Invariants (`core/hal/common/cpu_features.c`)**: Publication is too permissive. Allows duplicate publications per CPU, resets during collection, and freezes without ensuring all expected CPUs are present. `SYSTEM_ALL` could mean "all CPUs that happened to publish" instead of "all schedulable CPUs". Lifecycle APIs return `bool` instead of typed status codes.
* **Memops Failure Behavior**: Backend registration failure is silently ignored. Scalar fallback is used to inappropriately hide true configuration failures (like invalid backend, wrong lifecycle).
* **Userspace Architecture Split in BharatLibC**: `core/lib/bharatlibc/src/arch/memory.c` mixes generic code and architecture-specific (`#if BH_LIBC_ARCH_X86_64`) code, which recreates architecture mixing at the userspace level.
* **CPU-Local Header (`core/kernel/include/bharat/cpu_local.h`)**: Still reads like a prototype. Includes heavy transitive dependencies, embeds `sched_rq_t`, has declarations outside include guards, and contains "TODO" comments. Violates strict isolation.
* **CI/Quality Gates**: The build step in CI swallows errors by using `|| true`. The nightly performance gate has no functional thresholds. Missing ARM32/RISC-V32 cross-architecture compile coverage in PR-fast jobs.

### Other Significant Issues
* **Unsafe Struct Copying**: Scattered usage of raw pointer casting for copying (e.g., `((uint64_t *)dst)[i] = ...`) rather than standard assignments `*dst = *src;` or explicit Tier-0 raw byte-copy primitives in early boot to prevent compiler-generated SIMD.
* **Memops Metadata**: HAL backend properties (`implementation_flags`) are disconnected from constraint checking (`context_flags`), risking future NO_SIMD/NO_DMA safety if attributes aren't manually duplicated.
* **x86 Memops Metadata Inconsistency**: ERMS settings and `GPR_ONLY` flags mismatch.
* **Arch Implementation Naming**: Arch-specific implementations wrongly prefixed with `hal_` (e.g., `hal_memcpy_x86_gpr`). Should be `arch_x86_memcpy_gpr`.
* **RISC-V Memops Strict Aliasing**: Memops converts arbitrary byte buffers to `uint64_t*`, risking strict aliasing C violations.
* **UAPI Leakage**: Userspace feature descriptors (`bharat_cpu_features_v1_t`) are housed in `bharat/libc/memops.h` instead of a stable UAPI header.
* **Costly Atomic Acquires**: Libc memory operations perform atomic acquires on every call, causing synchronization overhead.
* **Dangerous `SYSTEM_ANY` Usage**: `arch_cpu_has_system_any()` used for local execution decisions (e.g., crypto routing) instead of broad system scheduling.
* **Testing Deficits**: Feature/memops tests mostly hit the generic dispatcher but miss testing failure paths, invalid lifecycles, and actual architecture backend behavior.

---

## 4. Execution Plan for Fixes

The following prioritized plan will stabilize the capabilities and memops architecture. Further optimization (AVX, NEON, SVE, DMA memcpy, more crypto) should be paused until this baseline is mechanically verified.

### P0 — Critical Lifecycle & Safety (CAPLIFE-001, HMEM-001, LIBC-CRT-001, X86CAP-001)
1. **CPU Capability Lifecycle Transaction (CAPLIFE-001)**
   - Refactor `core/arch/common/cpu_caps_state.c`, `hal_cpu_features.*`, and memops lifecycles into explicit states (`UNINITIALIZED`, `COLLECTING`, `VALIDATED`, `FROZEN`).
   - Validate expected CPU mask/count, support non-zero BSP logical IDs, and reject duplicate publications.
   - Enforce `usable ⊆ raw` capabilities.
   - Ensure finalization is an all-or-nothing transaction. Ensure g_present checking on CPU queries. Remove any generic `memset` usage.
   - Add fault-injection tests for state transitions to confirm no partial freeze is possible.

2. **Fail-Closed DMA/Cache Coherency (HMEM-001)**
   - Remove the `SEQ_CST` fence fallback from `hal_hmem_sync_range()`.
   - Ensure the API returns `K_ERR_UNSUPPORTED` if no backend is present, unless the platform explicitly declares global coherency.
   - Implement single-assignment backend installation using CAS.

3. **Real HWCAP Startup Wiring (LIBC-CRT-001)**
   - Remove the CMake build-time modification (`file(WRITE)`) in `core/lib/bharatlibc/CMakeLists.txt`.
   - Move `bharat_cpu_features_v1_t` to a stable UAPI path (`interface/include/bharat/uapi/hwcap.h`).
   - Implement exactly-once dispatch initialization in CRT and test standalone behavior.

4. **Raw-vs-Usable Closure (X86CAP-001)**
   - Audit all x86 capability features.
   - Enforce required OS state checks for usable features (CR4 controls, XCR0 ownership, and kernel policies).
   - Replace unsafe crypto `SYSTEM_ANY` usage with ALL/current/per-CPU scoped querying.

### P1 — Contract Hardening & Quality (MEMOPS-002, LIBC-MEMOPS-002, CPULOCAL-001, QUALITY-001)
5. **Backend Contract Hardening (MEMOPS-002)**
   - Upgrade boolean returns on lifecycle hooks to typed `kstatus_t`.
   - Distinctly handle deliberate fallback vs. initialization/registration failures.
   - Make backend traits authoritative instead of replicating to contexts.
   - Rename Arch implementations to `arch_*` and abstract them through Arch-private headers.

6. **Userspace Architecture Split (LIBC-MEMOPS-002)**
   - Split `core/lib/bharatlibc/src/arch/memory.c` into precise per-ISA directories (e.g., `src/arch/x86_64/`).
   - Retain generic fallback in core.
   - Remove atomic acquire sync from all hot libc calls, leveraging a one-time startup freeze model.

7. **CPU-Local Contract Cleanup (CPULOCAL-001)**
   - Define a single global authority, e.g., `BH_MAX_CPUS`, eliminating redundant CPU limit macros.
   - Clean up `cpu_local.h` by removing excessive dependencies, moving declarations inside include guards, and fixing comments/prototypes.

8. **Make Gates Trustworthy (QUALITY-001)**
   - Fix CI: Ensure the CMake build actually enforces failures rather than silencing with `|| true`.
   - Add ARM32/RISC-V32 compilation checks to PR-fast pipelines.
   - Extend the HAL dependency script (`check_hal_dependency_direction.py`) to block `Arch -> Kernel` violations.
   - Expand unit tests targeting failure flows and real Arch memops conformance.
