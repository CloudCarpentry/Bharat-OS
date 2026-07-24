---
title: Power and Thermal Governance for Bharat-OS
status: Proposed
owner: TBD
reviewers: TBD
version: 0.1
last_updated: 2024-05-24
tags: power, thermal, architecture
---

# Profile-Driven, Capability-Governed Power and Thermal Architecture for Bharat-OS

## 1. Context

Bharat-OS is aiming to support:

* small embedded devices
* mobile and battery-powered devices
* drones and robots
* laptops and desktops
* edge/server-class boards
* multi-kernel and future distributed-kernel deployments

Across these targets, the OS needs some form of:

* CPU power management
* board and SoC thermal control
* idle/suspend behavior
* power-domain control
* battery and charger coordination where applicable
* emergency protection for over-temperature, brownout, and critical battery states

At the same time, Bharat-OS must keep:

* the core kernel small
* the design hardware-agnostic
* policy outside the kernel where possible
* hard safety inside trusted low-latency paths
* support for RT, GP, and MIX profiles
* clean integration with capability model, IPC, and URPC

A monolithic kernel-only power stack would make the system too large, board-specific, and hard to evolve. A purely user-space design would miss hard real-time safety and low-latency control paths.

So we need a layered design.

## 2. Decision

Bharat-OS will implement power and thermal management as a layered resource governance subsystem with the following split:

**Kernel core owns**
* generic registries and abstractions
* idle/perf/suspend primitives
* energy telemetry hooks
* thermal trip and emergency safety paths
* capability enforcement
* scheduler-facing QoS and utilization hooks

**HAL and board/platform layers own**
* architecture- and board-specific implementation details
* SoC power states
* CPU idle/frequency backends
* PMIC/regulator/charger/battery/fan/sensor integration
* platform wake/suspend details

**Services own**
* performance vs efficiency policy
* product-specific policy
* fan curves
* battery and charger strategy
* drone mission reserve logic
* adaptive throttling policy
* optional AI-assisted optimization

**Multi-core/kernel/distributed coordination uses**
* IPC for local policy-control paths
* URPC for cross-kernel or cross-partition power/thermal budget coordination

This follows the rule:

**Kernel = mechanism and safety**
**Service = policy and product behavior**

## 3. Goals

**Primary goals**
* keep kernel minimal and portable
* support a wide hardware spectrum
* unify CPU, board, and battery-related governance under one generic model
* support RT, GP, and MIX profiles cleanly
* allow compile-time trimming for tiny devices
* allow optional advanced policy on larger devices
* make thermal and power safety first-class trusted behavior

**Secondary goals**
* enable future AI-governed optimization as an optional layer
* support remote/partitioned governance for multi-kernel systems
* provide clean observability and telemetry for testing and tuning

## 4. Non-goals

The core kernel will not own:

* battery chemistry algorithms
* OEM fan curve logic
* mobile charging policy UX decisions
* drone mission planning logic
* vendor-specific policy heuristics
* large ML/AI models
* rich user-facing power mode policy

These belong in services or platform policy modules.

## 5. Core architectural model

### 5.1 Kernel abstractions

The kernel will expose a generic model based on:

**Power domains**

Controllable energy islands such as:

* CPU core
* CPU cluster
* GPU
* NPU
* radio
* storage controller
* display block
* camera ISP
* sensor hub
* board rail / PMIC-backed domain

Typical states:
* ON
* CLOCK_GATED
* RETENTION
* SUSPEND
* OFF

**Performance domains**

Entities with performance states:
* CPU cluster DVFS domain
* accelerator performance island
* memory controller bandwidth/perf domain

**Idle states**

For CPU/cluster/system:
* busy
* shallow idle
* deep idle
* retention
* suspend

**Thermal zones**

Logical zones such as:
* CPU package
* cluster 0
* PMIC
* battery
* board/chassis
* RF subsystem
* motor controller
* skin temperature

**Cooling devices**

Anything that can reduce heat:
* fan PWM
* CPU throttling
* disable boost/turbo
* cluster cap
* radio TX power reduction
* charger current reduction
* display dimming
* disable noncritical accelerator
* drone thrust ceiling reduction

**Power QoS**

Request aggregation for:
* minimum performance
* maximum latency
* no-deep-idle
* thermal preference
* power budget hints

**Emergency conditions**

Trusted fast-path handling for:
* hard over-temperature
* brownout
* battery critical
* PMIC/regulator faults
* thermal runaway suspicion

### 5.2 Layer split

**Kernel**
Minimal common machinery.

**HAL/arch**
ISA/SoC primitives:
* WFI/WFE/HLT or equivalent
* cpuidle backend
* cpufreq or OPP hooks
* package energy counters if available
* architecture-local thermal interfaces if relevant
* low-level suspend entry/exit

**HAL/platform or HAL/board**
Board-specific pieces:
* PMIC
* charger IC
* battery/fuel gauge
* fan
* PWM cooling
* thermal sensors
* EC/BMC proxy
* drone rail monitor
* regulator tree
* wake source wiring

**Drivers**
Reusable device-class drivers.

**Services**
Policy daemons / trusted services.

## 6. Proposed directory structure

```
core/kernel/
  include/
    power/
      power.h
      power_domain.h
      perf_domain.h
      cpuidle.h
      cpufreq.h
      thermal.h
      cooling.h
      power_qos.h
      suspend.h
      energy.h
      power_events.h
    battery/
      battery_events.h
  src/
    power/
      power_core.c
      power_domain.c
      perf_domain.c
      cpuidle.c
      cpufreq.c
      power_qos.c
      suspend.c
      wakeup.c
      energy_accounting.c
      thermal_core.c
      thermal_trip.c
      cooling_device.c
      emergency_power.c

corecore/hal/
  common/
    power/
      null_power.c
      null_thermal.c
  arm32/
    power/
    thermal/
  arm64/
    power/
    thermal/
  riscv32/
    power/
    thermal/
  riscv64/
    power/
    thermal/
  x86_64/
    power/
    thermal/
  core/platform/
    qemu/
    stm32/
    rpi/
    drone_fc/
    mobile_ref/
    laptop_ref/
    server_ref/

core/drivers/
  power/
    regulator/
    pmic/
    battery/
    fuel_gauge/
    charger/
    clock/
    reset/
  thermal/
    sensor/
    fan/
    pwm_cooling/
    dvfs_cooling/

core/services/
  core/
    powerd/
    thermald/
    healthd/
  device/
    batteryd/
    chargerd/
    boardmgmt/
  mobility/
    dronemgr/
  datacenter/
    bmc_proxy/

profiles/
  rt/
    power_profile_rt.c
  gp/
    power_profile_gp.c
  mix/
    power_profile_mix.c
```

## 7. Kernel responsibilities

The kernel should only provide these families.

### 7.1 Power domain registry
Register and manage generic power domains and their capabilities.

### 7.2 Performance domain registry
Expose performance states and min/max/floor/cap constraints.

### 7.3 Idle framework
Provide common idle-state registration and residency accounting, with arch enter/exit callbacks.

### 7.4 Thermal core
Register zones, trips, cooling bindings, and thermal event notification.

### 7.5 Power QoS
Allow scheduler, drivers, and services to express constraints without embedding policy.

### 7.6 Suspend/wakeup core
Provide ordered system/device suspend and wake primitives.

### 7.7 Energy telemetry
Expose counters and residency/accounting data.

### 7.8 Emergency protection
Trusted low-latency response to dangerous power/thermal conditions.

### 7.9 Capability enforcement
Restrict who can:
* alter performance policy
* bind cooling policy
* request suspend
* modify charger behavior
* trigger emergency board actions

## 8. Service responsibilities

### 8.1 powerd
Owns:
* power profiles
* balanced/performance/eco policy
* performance domain policy
* autosuspend/device-runtime policy
* QoS aggregation policy
* suspend policy

### 8.2 thermald
Owns:
* thermal trip configuration
* cooling strategy
* fan curves
* staged throttling
* graceful degradation policies

### 8.3 healthd
Owns:
* sensor sanity checking
* telemetry logging
* health alerts
* anomaly detection
* fault event aggregation

### 8.4 batteryd
Optional on battery-powered devices:
* state-of-charge
* state-of-health
* cycle count
* reserve estimation
* discharge modeling

### 8.5 chargerd
* charger IC policy
* charging current control
* USB-PD / external source coordination
* thermal-aware charging reduction

### 8.6 dronemgr
For drone/robot profiles:
* reserve budget
* return-to-home reserve
* compute-vs-flight power tradeoff
* motor/brownout risk handling
* mission-aware throttling

## 9. Profile behavior

### 9.1 RT profile
**Priorities:**
* deterministic latency
* bounded state changes
* minimal jitter

**Typical behavior:**
* fixed or narrow performance window
* shallow idle preferred
* deep idle restricted
* aggressive DVFS disabled or tightly bounded
* thermal mitigation must be deterministic
* AI governor disabled by default

### 9.2 GP profile
**Priorities:**
* efficiency and UX balance

**Typical behavior:**
* full DVFS
* deep idle enabled
* autosuspend of devices
* full thermal policy
* optional AI-assisted power optimization

### 9.3 MIX profile
**Priorities:**
* preserve RT guarantees while allowing elastic GP behavior

**Typical behavior:**
* RT cluster/core budget protected
* GP cluster absorbs elastic throttling
* shared package thermal budget mediated
* distributed or cross-kernel broker may arbitrate headroom

## 10. Capability model

Power and thermal control must be capability-governed.

Illustrative capability families:
* `CAP_PWR_READ_TELEMETRY`
* `CAP_PWR_SET_PERF_LIMIT`
* `CAP_PWR_SET_DOMAIN_STATE`
* `CAP_PWR_REQUEST_SUSPEND`
* `CAP_THERM_READ_ZONE`
* `CAP_THERM_SET_TRIP`
* `CAP_THERM_BIND_COOLING`
* `CAP_BATTERY_READ_STATUS`
* `CAP_BATTERY_CONTROL_CHARGE`
* `CAP_BOARD_EMERGENCY_ACTION`

This fits the Bharat-OS governance model and prevents uncontrolled access by normal applications.

## 11. IPC and URPC integration

**Local single-kernel systems**
Use IPC between:
* kernel brokers
* powerd
* thermald
* batteryd
* board services

**Multi-kernel / partitioned systems**
Use URPC for:
* shared package thermal budget
* RT-vs-GP power arbitration
* remote battery/PMIC ownership
* distributed suspend coordination
* node-to-node budget propagation

Example:
RT kernel owns motor-control and reserve budget
GP kernel owns vision/UI
broker enforces guaranteed RT floor and flexible GP ceiling

## 12. AI governor position

AI-assisted optimization should be optional and never part of the hard safety path.

Recommended place:
* inside powerd
* inside thermald
* or as sibling service aigovd

It may:
* predict workload spikes
* predict thermal rise
* optimize fan/throttle curves
* estimate battery usage
* recommend domain placement

It must not:
* bypass hard thermal trips
* bypass emergency shutdown
* directly override safety invariants
* control RT-critical timing without strict bounded rules

## 13. Small-kernel contract

The long-term contract should stay close to:

* `power_domain_*`
* `perf_domain_*`
* `cpuidle_*`
* `cpufreq_*`
* `power_qos_*`
* `thermal_zone_*`
* `cooling_device_*`
* `suspend_*`
* `energy_*`
* `emergency_power_*`

Everything beyond that belongs in HAL, drivers, or services.
