# Bharat-OS `services/netmgr`

## Role

Control plane for networking, specially important for a distributed-kernel future.

General network orchestration service.
Owns:
- **Interface State**: Lifecycle and management of network interfaces.
- **Route Control Plane**: Routing policy and distribution.
- **Offload Policy**: Determining which tasks are pushed to hardware.
- **RSS / Queue Steering**: Directing network queues.
- **Data-Plane Placement**: Placing data planes across cores.
- **Zero-Copy Buffer Policy**: Enforcing network buffer allocations without copies.
- **NIC Queue Ownership**: Direct authority over NIC queues.

## Status
* **Current:** Partial implementation.
  - In-memory interface/address/route/neighbor and driver-health modules are implemented.
  - IPC dispatcher handles control-plane opcodes and populates typed responses.
  - Main daemon loop remains scaffolded; full runtime wiring and strict capability enforcement are still in progress.
* **Roadmap:** Full policy orchestration, distributed control-plane integration, and production capability checks.
