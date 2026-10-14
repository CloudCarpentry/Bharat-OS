---
name: bharat-os-service-runtime
description: Apply when implementing, refactoring, or integrating Bharat-OS userland services, service managers, init sequence, bootstrap, or supervisor protocols.
---

# Bharat-OS Service Runtime Skill

## Role of Userland Services

In Bharat-OS's microkernel architecture, policy is strictly relegated to userland system services:
- `vm_manager`: Virtual memory policy, address space allocation, memory reservation, paging policy.
- `process_manager`: Process lifecycle, ELF loading, task spawning, capability space initialization, thread scheduling priorities.
- `servicemgr`: Service registry, dependency graph resolution, service supervision, heartbeat monitoring, restart backoff.
- `devmgr`: Device discovery, driver lifecycle, bus arbitration, interrupt capability delegation.
- `netmgr`: Network interface orchestration, route tables, protocol stack binding.

## Core Service Runtime Invariants

1. **Production Authority Adapters (No Stubs)**:
   - A service must install a real production kernel authority adapter at startup before entering its event loop.
   - For example, `vm_manager` must install the native authority adapter that binds service operations (`create_aspace`, `map`, `unmap`, `protect`) directly to kernel VM mechanisms.
   - Merely providing mock test adapters or returning `BH_STATUS_ERR_UNSUPPORTED` in production runtime violates the architecture.
2. **Real Capability Endpoints (No Dummy Handles)**:
   - Services must communicate through runtime-created capability endpoints received during bootstrap.
   - Hardcoded dummy endpoints (e.g. placeholder integer handles or zeroed endpoints) are strictly prohibited.
3. **Bootstrap Protocol & Handshake**:
   - Every system service follows the standard bootstrap sequence:
     ```
     Service entry (main.c)
       │
       ▼
     Initialize internal state (service_init())
       │
       ▼
     Install native kernel authority ops (e.g. bh_vm_install_native_kernel_ops())
       │
       ▼
     Connect to bootstrap endpoint (bharat_bootstrap_init())
       │
       ▼
     Register capability endpoint with name server / servicemgr
       │
       ▼
     Emit runtime marker (e.g. VM_MANAGER_READY / PROCESS_MANAGER_READY)
       │
       ▼
     Enter event loop (service_loop())
     ```
4. **Event Loop & Asynchronous Handling**:
   - Service request loops must be event-driven using IPC message queues, epoll-equivalent dispatchers, or bounded poll mechanisms.
   - Never use busy spin loops in a service. If no requests are pending, yield or block on the IPC endpoint.
5. **Supervisor Integration & Health**:
   - Respond to periodic supervisor heartbeat pings.
   - Signal state transitions cleanly: `STARTING`, `READY`, `DEGRADED`, `SHUTTING_DOWN`.
   - On unrecoverable error, log structured diagnostic output and exit cleanly so `servicemgr` can apply exponential restart backoff.

## Runtime Verification Markers

A service is only considered runtime-verified when its startup emits its canonical console marker under QEMU:
- `PROCESS_MANAGER_READY`
- `VM_MANAGER_READY`
- `SERVICEMGR_READY`
- `DEVMGR_READY`
- `NETMGR_READY`
