---
title: Bharat-OS Heterogeneous Compute Architecture and Roadmap
status: Draft
owner: Architecture Team
reviewers: Core Maintainers
version: 1.0
last_updated: 2024-03-24
tags:
  - architecture
  - compute
  - accelerator
  - ai
  - memory
---

# Bharat-OS Heterogeneous Compute Architecture and Roadmap

## 1. Executive Summary

Bharat-OS implements a heterogeneous compute control plane that enables CPU, GPU, NPU, DSP, and other accelerators to function together in a cooperative model, without compromising the strict multikernel boundary or embedding complex AI/ML frameworks into the kernel. The kernel remains minimalistic and manages only deterministic mechanisms: accelerator discovery, queueing, completions, memory sharing, IOMMU control, isolation, scheduling hints, and telemetry.

Policy decisions—such as model selection, graph compilation, and routing across backends—are strictly delegated to services and user-space runtime libraries. This preserves the repository boundary rules: the kernel enforces mechanisms, services manage policy, drivers interact with hardware, and stacks compose functionality.

## 2. Why this fits Bharat-OS

- **Aligns with repository boundaries**: Maintains the separation of `arch/`, `hal/`, `kernel/`, `drivers/`, and `services/`.
- **Extends the capability model**: Leverages existing multikernel principles, including strict ownership, capability-gated isolation, and profile-aware execution.
- **Natural extension of memory architecture**: Built upon existing memory domains, DMA structures, and IOMMU hardware isolation concepts rather than side-stepping them.
- **Future-proofs AI workloads**: Creates a scalable path for ML hardware acceleration without polluting the kernel with fast-moving framework specific logic.

## 3. Architectural Principles

### 3.1 Mechanism vs. Policy

**Kernel Owns (Mechanisms):**
- Accelerator registration and capability descriptors
- Job submission contracts and queueing
- Pinned/shared/secure memory primitives
- DMA/IOMMU mapping lifecycles
- Fence and completion primitives
- Telemetry, telemetry counters, and fault reporting surfaces
- CPU ISA capability exposure for core selection and fallback

**Services & Runtime Own (Policies):**
- Model loading, quantization, and tensor graph compilation
- Backend selection policy and execution routing
- Batching
- Thermal and power management tuning (via policy hints)
- Operator library implementation

### 3.2 No ML Frameworks in the Kernel

The kernel must **never** parse ONNX, TFLite, or equivalent tensor graph representations. The kernel interface accepts only compact execution descriptors (jobs) containing command buffers, memory handles, fences, and capabilities.

### 3.3 Capability-Gated Compute

Accelerator access is fundamentally capability-mediated. A process or task may only submit work to a device or bind memory domains if it possesses explicit cryptographic or software capability rights to use those resources.

### 3.4 Profile Truthfulness

If a target profile lacks hardware features like coherent DMA, IOMMU page-based isolation, or preemptible execution, the kernel API must truthfully expose this lack of capability rather than silently emulating a degraded and unsafe subset.

## 4. Recommended Repository Boundaries

### 4.1 Kernel (`kernel/`)

The minimal heterogeneous compute substrate.

- `kernel/include/accel/accel_types.h`
- `kernel/include/accel/accel_job.h`
- `kernel/src/accel/accel_core.c` (registration, descriptors)
- `kernel/src/accel/accel_queue.c` (submission, scheduling)
- `kernel/src/accel/accel_fence.c` (completions, sync)
- `kernel/src/accel/accel_telemetry.c`
- `kernel/src/accel/accel_fault.c`
- `kernel/src/mm/accel/` (or via standard DMA extensions)
- Hooks in `kernel/src/sched/` for fallback/hint handoff.

### 4.2 HAL (`hal/`)

Abstraction contracts only; no architecture implementations.

- `hal/include/hal/hal_accel.h`
- Hooks for DMA, IOMMU, and memory cache maintenance related to accelerator needs.
- Defines standard accelerator capability queries, queue programming hooks, and fence bindings.

### 4.3 Arch (`arch/`)

ISA/CPU feature detection and capability normalization.

- Normalizes hardware features (vector width, dot-product, fp16/bf16, SVE, AMX) into a portable software capability structure (`accel_discovery_t`).
- Supports CPU fallback routines by reporting deterministic compute capabilities.

### 4.4 Drivers (`drivers/accel/`)

Concrete implementations mapping HAL contracts to hardware.

- `drivers/accel/common/`
- `drivers/accel/gpu/`
- `drivers/accel/npu/`
- `drivers/accel/virt/` (virtual, paravirtual, or mock backend)

### 4.5 Services (`services/device/`)

Policy managers.

- `services/device/accelmgr/`: Discovers accelerators, publishes policy, tracks lifecycle and availability.
- `services/device/aigov/`: Evaluates thermal/power constraints, sets deadline latency classes, issues fallback hints.

### 4.6 Runtime and UAPI

- `lib/runtime/accel/`: User space model loading and queue binding.
- `uapi/capability/accel.h`: ABI contract for accelerator capability descriptors, fences, and shared structures.

## 5. Execution Model

### 5.1 Job Descriptor

The kernel-visible job contract remains intentionally minimal:
- **job id**: Unique identifier for tracking.
- **submitter capability**: Token granting access to target.
- **buffers**: Handles or memory tokens for I/O operations and pinned execution memory.
- **fences**: Completion and dependency sync points.
- **targets**: Bitmap representing allowed execution domains (e.g., specific CPU cores, GPU, NPU).
- **hints**: Prioritization, deadlines, latency classes, power budgets.

### 5.2 Routing Flow

1. User/runtime logic allocates/pins memory buffers.
2. Service/runtime (`aigov`, `accelmgr`) selects target backend.
3. Kernel intercepts the job, validating capability tokens and IOMMU domain restrictions.
4. Kernel dispatches the job via HAL/Driver to the hardware queue.
5. Device signals completion (IRQ/fence).
6. Kernel clears the fence, signaling endpoints.
7. Service decides on subsequent stages (e.g., fallback execution on CPU if NPU failed).

## 6. CPU Support and Fallback Capabilities

The system extracts deep ISA details into portable tags for scheduling and capability constraints:
- **x86_64**: AVX2, AVX-512, AMX, VNNI
- **ARM64**: NEON, SVE, SVE2, SME, BF16, DOTPROD
- **RISC-V**: RVV, Zba, Zbb, vendor extensions

These hardware features are queried at boot (via `hal_discovery`) and utilized by `aigov`/`accelmgr` to select execution paths.

## 7. Security and Teardown Containment

- **Isolation**: IOMMU contexts per job where hardware permits.
- **Memory**: Secure memory semantics (`MEM_ACCEL_SECURE`) for protected model buffers.
- **Fault Tolerance**: Hardened cancel and revoke code paths. Unresponsive or faulting accelerators will be reset, and jobs torn down deterministically without leaking physical pages.
