# Device Profiles and Use-cases

This document maps Bharat-OS architectural features to practical deployment classes and distinguishes **implemented baseline** vs **roadmap**.

## Profile model

Bharat-OS uses profile-oriented tuning to keep a shared kernel spine while changing policy and scale assumptions:

- **RTOS profile:** smallest footprint and bounded behavior.
- **EDGE profile:** constrained power/thermal envelopes with mixed criticality workloads.
- **DESKTOP/SERVER profile:** broader throughput and service composition goals.

## Cross-profile architectural strengths (current baseline)

1. **Capability-mediated authority**
   - Object access is represented by explicit capabilities and rights checks.
   - Delegation is constrained to subsets of existing rights.
2. **Messaging-centric IPC**
   - Endpoint IPC baseline for synchronous interactions.
   - URPC ring primitives for bounded asynchronous pathways.
3. **Driver boundary scaffolding**
   - Device framework and MMIO registration patterns.
   - Capability-gated mapping path for privileged accelerator MMIO windows.
4. **Multicore/multikernel direction**
   - Secondary-core boot hooks and per-core channel matrix baseline.
   - NUMA descriptor baseline for placement-aware extensions.

## Deployment mapping

### 1) Mobile and wearables

**Current fit**

- Capability isolation helps reduce blast radius for compromised components.
- Scheduler telemetry hooks + AI suggestion path can support power-aware policy in user space.

**Gaps / roadmap**

- Production DVFS/thermal control integration and power-model calibration.
- Hardening for sustained battery-constrained runtime behavior.

### 2) Robotics and drones

**Current fit**

- Message-passing architecture and bounded queues support deterministic control-plane behavior.
- Architecture portability (x86_64/riscv64/arm64 build surfaces) supports mixed platform development.

**Gaps / roadmap**

- Stronger real-time scheduling guarantees and admission control.
- More complete fault containment and recovery semantics for safety-critical subsystems.

### 3) Network appliances and edge gateways

**Current fit**

- Capability/driver boundary model supports service isolation.
- Baseline multikernel messaging is compatible with control/data plane separation patterns.

**Gaps / roadmap**

- Mature high-throughput network stack/data plane acceleration.
- Better asynchronous eventing and device-interrupt integration depth.

### 4) Data-center and clustered services

**Current fit**

- Multicore + NUMA scaffolding and URPC channels establish scale-out direction.
- Policy/mechanism separation (including AI policy boundary) supports operational control planes.

**Gaps / roadmap**

- Full distributed scheduler behavior across nodes.
- Production-grade memory management depth and service lifecycle orchestration.

### 5) Research and education platforms

**Current fit**

- Clear separation of baseline vs deferred components supports experimentation.
- ADR-driven governance provides traceability for design decisions.

## References and alignment

- Multikernel direction aligns conceptually with Barrelfish’s messaging-first multicore argument.
- Capability discipline aligns conceptually with seL4/L4-family authority modeling.

These are architectural influences rather than claims of implementation equivalence.
