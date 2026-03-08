# Real-time / Automotive / Robotics Specific Subsystems

This document defines subsystem contracts for EV, drone, robotics, and industrial Bharat-OS deployments.

## 1) Deterministic IPC Mode

Deterministic IPC is mandatory in safety and control loops.

- **Bounded latency queues**: queue depth and enqueue/dequeue budget are profile controlled.
- **Priority-aware wakeups**: waiter wake order honors thread priority class for RT lanes.
- **Lock avoidance**: prefer per-core queues and non-blocking state transitions in hot paths.
- **Deadline hooks**: subsystems may emit deadline events for scheduler/telemetry/AI policy plugins.

### Core-kernel wiring completed

- `ipc_async_request_t` now carries optional deterministic and QoS metadata.
- timeout handling in kernel IPC can call a weak `bharat_rt_deadline_timeout_hook(...)` bridge.
- scheduler wakeups support a priority hint through `sched_wakeup_with_priority(...)`.

These additions are backward compatible for non-RT profiles because the default APIs still exist and
new behavior is opt-in.

## 2) Safety Partitioning and Health Monitoring

Safety partitioning boundaries are modeled as **fault containment domains**.

- **Watchdog framework**: per-domain watchdogs with explicit kick cadence.
- **Subsystem health manager**: centralized domain-health state (OK / degraded / failed).
- **Fault containment domains**: isolate powertrain, chassis, autonomy, infotainment, industrial I/O.
- **Safe-mode boot profile**: reduces boot graph and starts only critical services.

## 3) Automotive / Field Bus Subsystem

The field bus layer starts with deterministic low-level bus support.

- **CAN / CAN-FD** transport hooks.
- **LIN** transport hooks.
- **Automotive Ethernet hooks later**: explicit registration seam (`subsys_automotive_register_ethernet_hook`) is present for TSN-aware transport integration.
- **Time sync hooks**: bind bus clocks to monotonic system time and keep last sync snapshot for health/diagnostics.

## 4) Fast Boot Subsystem

Fast boot supports safety and real-time lanes.

- **Staged init**: service activation by ordered stage with `subsys_automotive_run_boot_stage(...)`.
- **Service dependency graph**: explicit dependencies for startup validation.
- **Boot profile selection**: normal, safe mode, and RT-minimal.
- **Minimal boot lane for RT/safety mode**: starts only essential services.

## API Mapping

Kernel/subsystem APIs for this plan are implemented in:

- `subsys/include/bharat/automotive/automotive.h`
- `subsys/src/automotive.c`
- `kernel/include/ipc_async.h`
- `kernel/src/ipc/async_ipc.c`
- `kernel/src/ipc/ipc_timeout.c`
- `kernel/include/sched.h`
- `kernel/src/sched.c`

These interfaces are intentionally conservative and profile-driven so integrators can tune policy
without changing scheduler or bus-driver internals.
