# Distributed Kernel Implementation Plan (Automotive & Mobility)

This plan converts the automotive architecture direction into an executable engineering program for Bharat-OS kernel, subsystem, and OS layers.

## 1. Outcomes and Acceptance Criteria

### Primary outcomes

- deterministic control readiness under 1-2 seconds on target boards
- mixed-criticality domain isolation with verifiable boundaries
- robust domain-to-domain and node-to-node messaging
- secure boot and update pipeline suitable for automotive lifecycle
- architecture portability across ARM, RISC-V, SHAKTI, and x86 simulation

### Program-level acceptance criteria

- control loop jitter and latency budgets are measurable and within profile limits
- safety domain survives service/application domain failures
- protocol stack interoperability validated in HIL/SIL pipelines
- update/rollback process is testable and audit-logged

## 2. Workstream Structure

## WS-A: Microcore and Deterministic Scheduling

1. Introduce scheduler classes:
   - hard RT fixed-priority class
   - mixed-criticality class (budgeted)
   - service best-effort class
2. Add CPU/core isolation primitives and affinity enforcement.
3. Add bounded interrupt-off and preemption-off accounting.
4. Publish latency and jitter telemetry interfaces.

**Deliverables**

- scheduler policy API and profile mapping
- deterministic timing benchmark suite
- real-time regression gates in CI

## WS-B: Domain Runtime Framework

1. Implement domain manager lifecycle (start/stop/restart).
2. Define domain manifests for memory/device/IPC permissions.
3. Implement safety runtime hooks (watchdog chain, degraded mode table).
4. Implement control runtime hooks (periodic task model, watchdog heartbeats).

**Deliverables**

- domain runtime library
- domain policy manifest format
- fault-injection tests for restart and containment

## WS-C: Bharat Fabric IPC

1. Standardize endpoint model for:
   - sync request/reply
   - async queue channels
   - pub/sub telemetry streams
2. Add shared-memory queue transport with mailbox notifications.
3. Add RPMsg/VirtIO compatibility adapter for Linux companion mode.
4. Add service discovery registry and endpoint versioning.

**Deliverables**

- IPC API specification
- transport conformance tests
- Linux companion interoperability demo

## WS-D: Automotive Communication Stack

1. CAN/CAN-FD driver and socket/service abstraction.
2. LIN controller abstraction for low-speed peripheral buses.
3. TSN-capable Ethernet datapath baseline and timing hooks.
4. SOME/IP service discovery and method transport framework.

**Deliverables**

- protocol subsystem interfaces
- board-specific driver adaptation guide
- network determinism validation scripts

## WS-E: Boot and Secure Lifecycle

1. Multi-stage boot implementation with domain startup ordering.
2. Signed image verification and key management hooks.
3. Measured boot event log format.
4. OTA update framework with A/B slot support and rollback counters.

**Deliverables**

- boot chain reference implementation
- secure update toolchain integration
- recovery path test matrix

## WS-F: HAL and Hardware Support Expansion

1. Consolidate HAL contracts (timer, IRQ, DMA, clock, memory map).
2. Provide architecture adapters:
   - ARM Cortex-M/R/A families
   - RISC-V standard + SHAKTI integration
   - x86_64 simulation/QEMU support
3. Add board support package (BSP) template for automotive ECUs.

**Deliverables**

- HAL interface specification
- per-architecture bring-up checklist
- BSP template repository structure

## WS-G: Profiles and Deployment Modes

1. Add profile definitions:
   - `BHARAT_PROFILE_AUTOMOTIVE_SAFETY_RT`
   - `BHARAT_PROFILE_AUTOMOTIVE_CONTROL`
   - `BHARAT_PROFILE_AUTOMOTIVE_GATEWAY`
   - `BHARAT_PROFILE_AUTOMOTIVE_AI_EDGE`
2. Map profile -> scheduler, memory, IPC, protocol, and driver policies.
3. Support deployment modes:
   - native
   - Linux companion
   - hypervisor partition
   - distributed multi-ECU

**Deliverables**

- profile schema and build-time selectors
- runtime policy validator
- deployment mode compatibility matrix

## 3. Milestone Plan

### M0 (0-4 weeks): Foundation Freeze

- finalize architecture spec and profile schema
- freeze HAL and IPC minimal contracts
- define safety/control readiness metrics

### M1 (4-10 weeks): Deterministic Core

- scheduler classes and core isolation
- domain lifecycle manager
- staged boot path with safety/control first

### M2 (10-16 weeks): Companion and Protocol Baseline

- RPMsg/shared-memory companion channels
- CAN/CAN-FD baseline
- service runtime and diagnostics skeleton

### M3 (16-24 weeks): Security and Distributed Scale

- secure/measured boot chain
- OTA update with rollback
- TSN/SOME-IP baseline and gateway services

### M4 (24+ weeks): Production Hardening

- HIL validation across representative ECU topologies
- safety case evidence artifacts
- long-run reliability and fault-injection closure

## 4. Validation Strategy

- **SIL**: QEMU/x86 and reference architecture simulation tests
- **HIL**: ECU bench rigs for timing, bus traffic, and fault injection
- **Soak**: long-duration telemetry and restart resilience tests
- **Security**: image signing, tamper, rollback, and attestation checks

## 5. Risk Register (Top Items)

1. **Timing budget drift** under mixed-criticality loads
   - Mitigation: strict scheduler admission control and periodic budget audit.
2. **Protocol integration complexity** across CAN/LIN/TSN/SOME-IP
   - Mitigation: staged interface contracts and conformance tests.
3. **Hardware variance** across SoCs/boards
   - Mitigation: HAL contract tests + BSP certification checklist.
4. **Companion-mode fault leakage** from Linux domain
   - Mitigation: capability boundary audits + explicit channel quotas.

## 6. Ownership Model

- Kernel team: WS-A, WS-B, WS-E core
- Platform/HAL team: WS-F
- Networking/vehicle stack team: WS-D
- System integration team: WS-C, WS-G, validation orchestration
- Security team: WS-E crypto/update hardening and audit trail

## 7. Definition of Done (Program)

Program is done when:

- all four automotive profiles build and boot on at least one representative target
- deterministic scheduler criteria are met under stress tests
- secure update and rollback workflows pass end-to-end
- distributed multi-ECU reference deployment is demonstrated with documented telemetry and fault containment behavior
