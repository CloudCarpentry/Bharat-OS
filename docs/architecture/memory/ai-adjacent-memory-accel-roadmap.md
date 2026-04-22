# AI-Adjacent Memory & Accelerator Primitive Roadmap

## 1. Goal and Position

This document captures a strict Bharat-OS architecture stance:

- Keep **AI framework and model semantics out of kernel ABI**.
- Add only **AI-adjacent OS primitives** in kernel/MM/capability contracts.
- Place policy, orchestration, model/runtime logic in services and libraries.

This aligns with existing repository boundaries and current architecture direction:

- kernel = mechanisms
- drivers = hardware control
- services = policy and orchestration
- lib/runtime = backend and execution logic

## 2. Current Code Reality (as of this snapshot)

### 2.1 What is already present

1. **Base memory class taxonomy exists** (`MEM_NORMAL`, `MEM_DMA`, `MEM_RT`, `MEM_SECURE`, etc.), but no explicit tensor/model/stream classes yet.
2. **Kernel accelerator memory primitives exist** for buffer lifecycle, pin/unpin, scatter-gather generation, IOVA bind/unbind, and sync directions.
3. **Capability policy already includes accelerator capability families** (`CAP_TYPE_ACCEL_DEVICE`, `CAP_TYPE_ACCEL_QUEUE`, `CAP_TYPE_ACCEL_BUFFER`, `CAP_TYPE_ACCEL_TELEMETRY`, `CAP_TYPE_ACCEL_ADMIN`) with rights masks.
4. **`services/device/accelmgr` exists** with the right policy-level intent (admission, queue policy, thermal throttling, telemetry), but implementation is still minimal.
5. **Driver/runtime stubs exist** (`drivers/accel/virt_accel.c`, `lib/runtime/accel/tensor_dispatch.c`) indicating architectural direction is present, while production behavior is not yet complete.

### 2.2 Gaps relative to target design

1. **No explicit AI-oriented alloc classes** integrated into the top-level `alloc_class_t` contract.
2. **No generic cross-device kernel job/fence object contract** yet as a stable UAPI/IDL pair.
3. **Capability model needs quota/domain granularity** (queue classes, bandwidth budgets, model-compartment access, fault-domain tags).
4. **Telemetry and fault containment are partially represented but not fully normalized as a production contract across kernel/driver/service layers.**
5. **Roadmap sequencing is not yet written as memory-first + accel contract phases tied to correctness priorities.**

## 3. Architecture Split (normative)

## 3.1 Kernel/MM must own only mechanisms

- Accelerator-neutral submission object primitives
- Shared memory and zero-copy primitives
- Fences, synchronization, deadline metadata
- Memory-class allocation tags and mapping semantics
- Capability validation and isolation hooks
- Fault-domain tagging and deterministic teardown hooks
- Telemetry counters and query surfaces
- Scheduling hints only (not policy)

## 3.2 Keep out of kernel

- Model loading formats
- Graph compilation/fusion/partitioning policy
- Tokenizers/session/prompt lifecycle
- Quantization policy
- Framework-specific APIs
- Model-routing policy

## 3.3 Placement by folder

- `kernel/` and `kernel/src/mm/`: primitive contracts only
- `drivers/accel/...`: device queue programming, DMA, MMIO, interrupt completion
- `services/device/accelmgr/`: admission, routing, fallback, quotas, throttling
- `services/device/aigov/`: policy/governance and safety profile logic
- `lib/runtime/accel/`: graph/runtime/backend plugins and CPU fallback kernels

## 4. Proposed Primitive Additions (small, stable set)

### 4.1 Memory classes

Extend `alloc_class_t` with AI-adjacent classes designed as generic compute/data movement classes:

- `MEM_TENSOR`
- `MEM_TENSOR_PINNED`
- `MEM_MODEL_RO`
- `MEM_STREAM_DMA`
- `MEM_SECURE_MODEL`
- `MEM_SCRATCH_LOWLAT`
- `MEM_SHARED_ACCEL`

These are allocation/mapping semantics, not framework semantics.

### 4.2 Generic accelerator job primitive

Define a neutral job descriptor that supports:

- submit
- attach buffer handles
- attach fence/deadline
- completion/error signaling
- cancel/revoke
- fault-domain tag

The descriptor must avoid ML-specific fields.

### 4.3 Capability contract extensions

Build on current accel capability families with explicit resource dimensions:

- device access
- queue class access
- budget/quota dimensions (time, bandwidth, memory)
- memory region/domain bind permissions
- optional secure model compartment permissions

### 4.4 Data movement primitives

Add or standardize only low-level primitives with broad utility:

- optimized copy/fill/move variants
- scatter-gather chain helpers
- zero-copy ring handoff primitives
- cache maintenance hooks
- secure zeroization helper

Do not add ML operators to kernel.

### 4.5 Scheduling hints

Accept hints such as:

- latency-sensitive
- throughput-oriented
- deadline-bound
- power-capped
- memory-bandwidth-heavy
- accelerator-preferred

Policy decisions stay in services/plugin policy.

## 5. Implementation Roadmap (ticket-oriented)

### Phase 0 — Correctness-first contract freeze (P0)

1. Freeze accelerator-memory primitive scope and non-goals.
2. Publish normative kernel/service boundary table.
3. Add tests ensuring no ML/framework API enters kernel UAPI headers.

### Phase 1 — Memory and capability primitives (P0)

1. Add AI-adjacent alloc classes in `kernel/include/bharat/mem_class.h`.
2. Map new classes into PMM/VMM policy hooks without policy leakage.
3. Introduce capability rights/profile checks for queue-class and domain usage.
4. Add fail-closed validation for unsupported profile features.

### Phase 2 — Job/fence contract + minimal UAPI/IDL (P1)

1. Define neutral job/fence descriptor headers (`include/bharat/uapi/device/accel_*`).
2. Add kernel validation path (rights, buffer ownership, deadline metadata).
3. Add completion/error/fault-domain state machine tests.

### Phase 3 — Service/runtime integration (P1)

1. Expand `services/device/accelmgr/` into admission + routing broker.
2. Connect telemetry and fault reporting into service-facing interfaces.
3. Extend `lib/runtime/accel/` to select backend via capability + profile truthfulness.

### Phase 4 — Profile-aware production hardening (P2)

1. Add profile matrices:
   - mobile (thermal/battery-aware)
   - deterministic/safety profiles (auditability)
   - throughput profiles (desktop/cloud)
   - constrained IoT/DSP-first profiles
2. Add conformance tests per profile and per translation model.
3. Validate deterministic teardown and memory reclamation on accelerator faults.

## 6. Concrete first task list

1. Add design ADR: “Kernel AI-adjacent primitive scope and non-goals”.
2. Draft `accel_job` and `accel_fence` neutral UAPI structs.
3. Introduce new memory class enum entries and compatibility mapping table.
4. Add kernel self-tests:
   - class allocation admission
   - capability-right rejection paths
   - job cancel/revoke teardown
5. Extend `accelmgr` skeleton with:
   - queue admission checks
   - budget/accounting placeholders
   - telemetry surfacing path

## 7. Acceptance criteria

A change is accepted only if all are true:

1. Kernel diff adds only mechanisms, no framework/model policy.
2. New UAPI fields remain accelerator-neutral and profile-truthful.
3. Capability checks fail closed on missing rights/features.
4. Service/runtime layers own backend selection and AI policy behavior.
5. Tests cover rejection paths and fault teardown determinism.

## 8. Decision rule for future proposals

Use this gate:

- If useful without AI models, it may belong in kernel.
- If meaningful only through ML framework semantics, keep it in runtime/services.

This preserves kernel minimalism while still enabling efficient accelerator-backed compute.
