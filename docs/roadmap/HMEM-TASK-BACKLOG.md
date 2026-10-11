# HMEM Task Backlog

This backlog provides discrete, architecture-compliant tasks to deliver the Phase 0–Phase 2 HMEM objectives, designed for independent agents where possible.

## Phase 0: Real CPU-backed userspace HMEM

### HMEM-P0-01 — Convert HMEM backing into a proper kernel memory object
*   **Priority:** P0 | **Dependencies:** None
*   **Objective:** Eliminate `kmem_aligned_alloc` heap usage in the kernel registry in favor of PMM/VM mapped pages.
*   **Existing code:** `core/kernel/src/mm/hmem.c`
*   **Functional requirements:** Allocate, map, unmap, and destroy paths must rely on the canonical PMM. HMEM remains an object descriptor, not a heap itself.
*   **Invariants:** All pages correctly reference-counted.
*   **Acceptance/Tests:** Ensure zero leaked pages upon allocation failure, map/unmap routines, and destruction.
*   **Architecture matrix:** GP/Edge MMU-Full and MMU-Lite targets.
*   **Definition of Done:** Capability/VM paths correctly integrate without `kmem_aligned_alloc`.

### HMEM-P0-02 — Move HMEM ownership into process CSpace/capabilities
*   **Priority:** P0 | **Dependencies:** None
*   **Objective:** Shift security logic from global structs into formal CSpace capabilities.
*   **Existing code:** `core/kernel/src/mm/hmem.c`, `interface/uapi/capability/capability.h`
*   **Functional requirements:** Implement `CAP_TYPE_HMEM` with standard rights (READ/WRITE/EXEC/MAP). Ensure handle scoping properly ties to caller context.
*   **Invariants:** Processes cannot hijack capabilities by numeric identifier. Read-only handles cannot be escalated to map.
*   **Acceptance/Tests:** Tests verifying `CAP_TYPE_HMEM` rights constraints, specifically checking isolation constraints and revocation sequences.
*   **Definition of Done:** `bh_hmem_t` rights field drops legacy global rights check.

### HMEM-P0-03 — Remove the global HMEM registry
*   **Priority:** P0 | **Dependencies:** None
*   **Objective:** Eliminate `hmem_slots[64]` and replacing it with core-local ownership records, compliant with distributed/multikernel roadmap.
*   **Existing code:** `core/kernel/src/mm/hmem.c`
*   **Functional requirements:** Handled via canonical registry. Cross-core operations require uRPC.
*   **Invariants:** Mutable state not globally shared.
*   **Acceptance/Tests:** Concurrency validation testing object mutations.
*   **Definition of Done:** Global HMEM array and lock disappear completely.

### HMEM-P0-04 — Add Native HMEM ABI operations
*   **Priority:** P0 | **Dependencies:** HMEM-P0-01, HMEM-P0-02 (Contract-level)
*   **Objective:** Add `hmem_create`, `hmem_destroy`, `hmem_query`, `hmem_map_cpu`, `hmem_unmap_cpu`, `hmem_sync_for_cpu`, and `hmem_sync_for_device` to `native_syscalls.json`.
*   **Existing code:** `interface/contracts/abi/native_syscalls.json`
*   **Functional requirements:** Must generate standard dispatch stubs, accepting well-defined structs without exposing physical/kernel VAs.
*   **Invariants:** Struct size/version enforcement, user-copy boundaries strict.
*   **Acceptance/Tests:** ABI verifier tests.
*   **Definition of Done:** Native capabilities compiled successfully in all tiers.

### HMEM-P0-05 — Implement real userspace CPU mapping
*   **Priority:** P0 | **Dependencies:** HMEM-P0-04, HMEM-P0-01
*   **Objective:** Back `HMEM_MAP_CPU` with userspace VA mapping using VMM.
*   **Existing code:** `core/kernel/src/personality/native/native_syscall_handlers.c`, `hmem.c`
*   **Functional requirements:** VMM reservation/mapping giving an actual userspace VA to the user process. Ensure revocation correctly tracks mappings.
*   **Invariants:** Mappings correctly dropped upon object destroy/revocation. W^X memory protection.
*   **Acceptance/Tests:** Tests proving memory bounds faults catch overruns on userspace addresses.
*   **Definition of Done:** Process destruction handles HMEM VM drop gracefully.

### HMEM-P0-06 — Split the SDK into host and Bharat-native backends
*   **Priority:** P0 | **Dependencies:** HMEM-P0-04
*   **Objective:** Diverge the `posix_memalign` mock away from genuine syscall-driven HMEM on target systems.
*   **Existing code:** `interface/sdk/lib/memory/hmem.c`
*   **Functional requirements:** Provide `hmem_native.c` using standard SDK boundary stubs.
*   **Invariants:** Guests strictly execute syscalls, avoiding silent host emulation.
*   **Acceptance/Tests:** Build and link process validation.
*   **Definition of Done:** Two distinct logic paths exist based on target.

### HMEM-P0-07 — Complete the real process lifecycle path
*   **Priority:** P0 | **Dependencies:** HMEM-P0-05, HMEM-P0-06
*   **Objective:** Full integration of userspace apps requiring process_manager/vm_manager/kernel threading.
*   **Functional requirements:** Process boot sequences support HMEM access inherently.
*   **Acceptance/Tests:** Log extraction showing VM creation, CSpace initialization, test, and exit.
*   **Definition of Done:** Complete process orchestration demonstrated.

### HMEM-P0-08 — Build the first real application: `hmem_frame_pipeline`
*   **Priority:** P0 | **Dependencies:** HMEM-P0-06
*   **Objective:** Provide a testbench executing a video/tensor data pipeline (Producer -> Preprocess -> Tensor Inference) demonstrating zero-copy.
*   **Functional requirements:** Benchmark baseline vs HMEM usage, evaluating checksums.
*   **Definition of Done:** Checksums identical, memcpy cycles removed.

### HMEM-P0-09 — Upgrade benchmarking from “PASS” to quantitative evidence
*   **Priority:** P0 | **Dependencies:** HMEM-P0-08
*   **Objective:** Expose benchmark parameters inside `bench_hmem_tensor`.
*   **Functional requirements:** Measure copy times, map latencies, etc.
*   **Definition of Done:** Exported metadata formats correctly JSON/CSV metrics.

### HMEM-P0-10 — Add contention and SMP benchmarks
*   **Priority:** P0 | **Dependencies:** HMEM-P0-09, HMEM-P0-03
*   **Objective:** Benchmark multithread/multicore mapping capabilities.

### HMEM-P0-11 — Add security and destruction stress tests
*   **Priority:** P0 | **Dependencies:** HMEM-P0-05
*   **Objective:** Add robust fuzzing, capabilities-denial testing.
*   **Functional requirements:** Double-frees, stale mappings, cross-process poisoning scenarios.

## Phase 1 & 2 Tasks
*   **HMEM-P1-01**: Fix the device-mapping abstraction. (IOMMU path logic)
*   **HMEM-P1-02**: Require dual authority for device access (HMEM+DMA capabilities).
*   **HMEM-P1-03**: Implement virtual QEMU HMEM accelerator engine.
*   **HMEM-P1-04**: Add explicitly synchronized FENCE/timeline concepts.
*   **HMEM-P1-05**: Route `hmem_frame_pipeline` through virtual accelerator.
*   **HMEM-P2-01**: Hardware DMA provider on x86_64/ARM64.
*   **HMEM-P2-02**: Accel provider (GPU/NPU).
*   **HMEM-P2-03**: Full arch matrix extension (ARM32/RV32 logic validation).
*   **HMEM-P2-04**: CI validation bundle outputs.
