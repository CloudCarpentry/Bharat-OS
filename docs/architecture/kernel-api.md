# Bharat-OS Kernel API Reference

This document provides a high-level overview of the most critical Bharat-OS kernel APIs. This API prioritizes clear boundaries, bounded kernel mechanisms, and capability-driven object invocation.

## Inter-Process Communication (IPC)

The primary communication primitives, heavily influenced by L4 and Barrelfish.

### Endpoint Synchronous IPC (`<ipc_endpoint.h>`)
Fast register-based synchronous IPC endpoints for local system calls.

* `int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap)`
  - Creates an IPC endpoint and returns the corresponding send/receive capabilities.
* `int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len)`
  - Synchronous send on an endpoint. Requires a valid send capability.
* `int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t capacity, uint32_t* out_len)`
  - Synchronous receive. Blocks the thread until a message is received.

### Multikernel URPC (`<advanced/multikernel.h>`)
Lockless ring-buffer for cross-core multikernel messaging (User-level Remote Procedure Call), modeled after Barrelfish.

* `int urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size)`
  - Initializes a URPC ring on top of a backing message buffer.
* `int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg)`
  - Lockless send to a URPC ring.
* `int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg)`
  - Lockless receive from a URPC ring. Does not block; returns `URPC_ERR_EMPTY` if no message is pending.

## Threading and Scheduling

Bharat-OS utilizes bounded scheduling mechanics with optional AI suggestion queues.

### Threads (`<sched.h>`)
* `kthread_t* sched_create_thread(address_space_t* as, void* entry_point, uint32_t priority)`
  - Creates a thread in the specified address space.

### AI Governor Integration (`<advanced/ai_sched.h>`)
The AI scheduler collects telemetry via PMC/approximations and consumes suggestions (see Korshun et al. 2024).

* `void ai_sched_update_telemetry(ai_sched_context_t* ctx, uint64_t cycles, uint64_t inst)`
  - Records execution counts to determine CPI.
* `int sched_enqueue_ai_suggestion(const ai_suggestion_t* suggestion)`
  - A bounded ingest queue for actions like `AI_ACTION_MIGRATE_TASK`, `AI_ACTION_ADJUST_PRIORITY`, etc.

## Virtual Memory Management (VMM)

The kernel primarily manages physical pages mapping to root tables; policy remains in user space. (L4 Minimalist Model).

### Low-level Architecture VMM (`<hal/vmm.h>`)
* `phys_addr_t hal_vmm_init_root(void)`
  - Allocates and initializes an architecture-specific root page table.
* `int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags)`
  - Inserts a mapping directly into the specified root table.
* `int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr)`
  - Removes a mapping.

### High-level VMM (`<mm.h>`)
* `int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags)`
  - Maps to the kernel's active address space.
* `int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu)`
  - Authenticates via capabilities (`CAP_RIGHT_DEVICE_NPU`/`GPU`) before mapping device memory.
