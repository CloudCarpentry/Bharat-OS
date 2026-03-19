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
* **Current:** Architectural stub.
* **Roadmap:** Policy enforcement, interface capability delegation, and orchestration agent.
