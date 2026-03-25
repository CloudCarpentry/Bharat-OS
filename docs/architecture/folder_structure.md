---
title: Bharat-OS Project Folder Structure
status: Draft
version: 1.0
owner: Architecture Team
reviewers: Core Maintainers
last_updated: 2024-03-24
tags:
  - architecture
  - structure
  - boundaries
  - repository
---

# Bharat-OS Project Folder Structure

This document outlines the architectural boundaries and exact directory structure for the Bharat-OS repository. The goal of this structure is to maintain clear semantic separation between architecture, abstraction, platform integration, services, and the core kernel.

*Note: The repository is currently partially aligned with this folder structure, and migrations of `services/` components (e.g., to `services/device/`, `services/core/`) are ongoing.*

---

## Architectural Boundaries

The core philosophy of Bharat-OS folder structuring is **semantic sharpness**:
- One folder = one responsibility.
- No overlapping classification systems.
- No ambiguous names.
- No policy creeping back into kernel/hal/arch.

### 1. The `arch/`, `hal/`, and `platform/` Boundary

This is the most critical separation in the low-level system.

- **`arch/`**: Owns ISA/CPU-specific code. This contains the hardware-specific implementations of contracts. (e.g., `arch/arm/arm64/`, `arch/x86/x86_64/`).
- **`hal/`**: Owns abstraction contracts and common glue. It contains only headers (`include/`) defining the HAL APIs, and generic implementations or stubs (`common/`, `irq/`, `timer/`, `memory/`, etc.). **Do not put architecture-specific implementations (like `hal/arm64`) here.**
- **`platform/`**: Owns board, machine, SoC integration, and topology/interconnect discovery. (e.g., `platform/boards/`, `platform/soc/`, `platform/qemu/`).

*Rule of Thumb:* `arch/` contains the implementations of the contracts defined in `hal/`. `platform/` wires them together for a specific physical or virtual machine.

### 2. The `services/` Hierarchy

`services/` is a first-class layer for policy and system-level managers. To prevent it from becoming a "catch-all" dumping ground, it is strictly categorized:

**Heterogeneous Compute Rule**:
> Kernel owns deterministic compute mechanisms (queues, jobs, memory isolation).
> Services own routing policy, fallback logic, and model selection.
> Drivers own accelerator hardware control.
> Runtime/lib owns model/backend logic and graph compilation.

- **`core/`**: Core managers (e.g., `init/`, `coremgr/`, `devmgr/`, `capmgr/`).
- **`system/`**: System utilities (e.g., `console/`, `diag/`, `logd/`, `boot_displayd/`).
- **`security/`**: Security services (e.g., `crypto/`, `keystore/`).
- **`device/`**: Device-specific policy managers (e.g., `accelmgr/`, `aigov/`, `actuator_mgr/`, `sensormgr/`).
- **`network/`**: Network daemons/managers (e.g., `can/`, `netmgr/`, `gatewayd/`).

*Note:* Real device drivers belong in `drivers/`, not `services/`. Only driver-facing service adapters belong in `services/`.

### 3. The `personalities/` Boundary

Personalities represent external interfaces, ABIs, or domain profiles. They are separated to avoid mixing compatibility with domain policy.

- **`compat/`**: OS compatibility and ABI personalities (e.g., `android/`, `linux/`, `windows/`).
- **`domain/`**: Domain or runtime profile policies (e.g., `automotive/`).
- **`common/`**: Shared personality utilities.

### 4. The `kernel/` Scope

The kernel is intentionally kept minimal. Anything policy-heavy should be questioned before staying in `kernel/`.

The `kernel/` directory should only contain:
- Core scheduling mechanisms
- Memory mechanisms
- Syscall/trap mechanisms
- Capabilities
- IPC/uRPC primitives
- Fault handling
- Minimal coordination primitives

### 5. `stacks/` and `uapi/`

- **`stacks/`**: For composed subsystems that span multiple layers (e.g., `net/`, `storage/`, `media/`, `vehicle/`, `cloud/`). This prevents these complex systems from being smeared across `services/`, `lib/`, and `drivers/`.
- **`uapi/`**: The explicit boundary for external contracts. Contains syscall headers, capability invoke contracts, shared IPC structures visible outside the kernel, and stable ABI types.

---

## Target Folder Structure

```text
Bharat-OS/
  arch/
    common/
    arm/
      common/
      arm32/
      arm64/
    riscv/
      common/
      riscv32/
      riscv64/
    x86/
      x86_64/

  boot/
    common/
    discovery/
    protocols/

  hal/
    include/
    common/
    irq/
    timer/
    ipi/
    dma/
    iommu/
    console/
    memory/

  platform/
    common/
    qemu/
    boards/
    soc/
    fabric/

  kernel/
    include/
    src/
    selftest/

  drivers/
    bus/
    net/
    block/
    display/
    input/
    serial/
    storage/
    accel/
      common/
      gpu/
      npu/
      virt/
    sensor/

  services/
    core/
    system/
    security/
    device/
      accelmgr/
      aigov/
    network/

  personalities/
    compat/
      android/
      linux/
      windows/
    domain/
      automotive/
    common/

  lib/
    ipc/
    msg/
    net/
    packet/
    posix/
    runtime/
      accel/
    syscall/
    transport/
    urpc/
    elf/

  stacks/
    net/
    storage/
    vehicle/
    media/

  uapi/
    syscall/
    capability/
    ipc/
    compat/

  idl/
    capability/
    monitor/
    services/
    transport/
    versioning/

  tests/
  tools/
  docs/
  staging/
```
