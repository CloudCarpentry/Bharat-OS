---
title: Safety-Oriented Profile Roadmap
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - roadmap
  - safety
see_also:
  - README.md
  - docs/architecture/device-profiles-and-use-cases.md
---
# Safety-Oriented Profile Roadmap

This document outlines the architectural roadmap for Bharat-OS safety-oriented profiles, focusing on the separation of concerns across the kernel, services, stacks, and personalities.

## Architectural Allocation

To maintain a minimal and verifiable trusted computing base, Bharat-OS strictly allocates functionality across layers:

### 1. What belongs in the Kernel
- **Minimal mechanisms only:** Scheduling (including deadline primitives), memory mapping, syscall/trap handling, capability management, IPC/uRPC transport, and fault-domain tagging.
- **Goal:** Keep the kernel policy-free and as small as possible.

### 2. What belongs in Services
- **System Policy:** Safety Manager, Fault Manager, Device Manager, and Service Supervisor.
- **Resource Management:** Policy-heavy behavior such as restart/backoff logic, watchdog monitoring, and telemetry aggregation.

### 3. What belongs in Stacks
- **Subsystem Logic:** Composed network stacks (e.g., TSN, SOME/IP), storage subsystems, and vehicle/sensor stacks.
- **Goal:** Provide reusable domain-specific logic that runs in user-space.

### 4. What belongs in Personalities / Domain
- **Application-Specific Mapping:** Domain profiles such as `AUTOMOTIVE`, `INDUSTRIAL`, or `AEROSPACE`.
- **Goal:** Define the specific composition of services and stacks required for a particular use case.

## Safety Maturity Levels (S-Levels)

Bharat-OS uses a specific maturity lens for safety profiles, aligned with the existing repository taxonomy.

| Safety-profile maturity | Description | Existing repo taxonomy alignment |
| --- | --- | --- |
| **S0 Scaffold** | Build profile exists; architecture defined but no runtime guarantees. | **Scaffold** |
| **S1 Baseline** | Boots and runs with profile-specific service selection; basic isolation. | **Baseline** |
| **S2 Bounded** | Key IPC, timer, and memory paths have bounded behavior tests and deterministic traces. | **Partial → Baseline+** (depending on tests) |
| **S3 Fault-contained** | Services and drivers have explicit fault domains, restart policies, and safe-state transitions. | **Production-candidate prerequisite** |
| **S4 Cert-prep** | Full traceability, timing evidence, safety case documentation, and hardware qualification artifacts. | **Beyond Production taxonomy; certification stage** |

> **Note:** The current repository state is mostly **S0/S1** depending on the specific profile.

## Maturity Gaps and Roadmap

### Phase 1: Foundation (Current Focus)
- Formalize `fault_domain_id` and tagging in kernel.
- Implement fail-closed capability shims in all service entry points.
- Define the `Safety Manager` contract and event structures.

### Phase 2: Deterministic Primitives
- Implement bounded execution paths for core IPC and memory operations.
- Add deadline-aware scheduling metadata to thread primitives.
- Introduce timing-trace validation in CI/CD.

### Phase 3: Fault Containment & Recovery
- Enable service-level restart with backoff and state recovery.
- Implement the `Safe-State` transition manager.
- Formalize driver isolation and recovery via IOMMU/capability mediation.

### Phase 4: Certification Readiness
- Generate automated traceability reports between requirements and tests.
- Complete hardware-in-the-loop (HIL) validation for target boards.
- Finalize safety case artifacts for selected profiles.

## Important Caveat
Bharat-OS is currently in an architecture-direction and prototyping phase. **S-levels do not equate to production safety certification.** Production deployments require additional validation, fault-containment analysis, and hardware-specific qualification.
