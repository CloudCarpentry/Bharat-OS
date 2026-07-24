---
title: Phase 1 Implementation Task Pack: Power and Thermal Governance
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Phase 1 Implementation Task Pack: Power and Thermal Governance

This document outlines the Phase 1 implementation tasks for the Power and Thermal Governance subsystem in Bharat-OS. The tasks are designed to build out the minimal kernel framework and null backends, enabling a stable foundation for advanced policy services in later phases.

## Ticket 1 — Add core power/thermal headers and null framework

**Scope:**
* create `core/kernel/include/power/`
* create `core/kernel/include/battery/`
* define generic enums, IDs, capability flags, and small core structs
* add `corecore/hal/common/power/null_power.c` and `corecore/hal/common/power/null_thermal.c`
* no real policy yet

**Acceptance Criteria:**
* kernel builds with framework enabled
* null backend compiles on all targets
* no behavior changes on unsupported boards

---

## Ticket 2 — Power domain registry

**Scope:**
* implement `core/kernel/src/power/power_core.c` and `core/kernel/src/power/power_domain.c`
* support registration, lookup, capability query, state query/set
* maintain generic, board-agnostic model

**Acceptance Criteria:**
* domains can be registered by HAL/platform code
* unsupported operations degrade cleanly
* tests cover registration, invalid transitions, capability checks

---

## Ticket 3 — Thermal zone and cooling device registry

**Scope:**
* implement `core/kernel/src/power/thermal_core.c`, `core/kernel/src/power/thermal_trip.c`, `core/kernel/src/power/cooling_device.c`
* allow zone registration, trip registration, cooling registration
* basic zone-to-cooling binding

**Acceptance Criteria:**
* thermal zones and cooling devices register and query correctly
* trip metadata can be stored and inspected
* null backends keep system behavior stable

---

## Ticket 4 — Power QoS framework

**Scope:**
* implement `core/kernel/src/power/power_qos.c`
* support requests for min performance, max latency, no-deep-idle
* expose aggregated effective constraints to idle/perf code

**Acceptance Criteria:**
* multiple requesters can add/remove constraints
* aggregation rules are deterministic
* tests validate request lifecycle and merge behavior
