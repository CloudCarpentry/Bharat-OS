---
title: Drivers Subcomponents Architecture (Repository-Aligned Status + Roadmap)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - components
see_also:
  - README.md
---
# Drivers Subcomponents Architecture (Repository-Aligned Status + Roadmap)

This document maps driver domains to the **current `core/drivers/` folder tree** and captures concrete convergence tasks.

## Current repository-aligned driver map

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#283618','primaryTextColor':'#ffffff','lineColor':'#dda15e','fontFamily':'Inter'}}}%%
graph TD
    D[core/drivers/] --> BUS[bus]
    D --> NET[net]
    D --> STORAGE[storage]
    D --> DISPLAY[display]
    D --> INPUT[input]
    D --> SERIAL[serial]
    D --> ACCEL[accel]
    D --> CLASS[class/*]
    D --> CORE[core + registry]
    D --> DEVICES[devices/*]

    BUS --> I2C[i2c]
    BUS --> SPI[spi]
    BUS --> USB[usb]
    BUS --> GPIO[gpio]
    NET --> VNET[virtio_net]
    STORAGE --> NVME[nvme]
    DISPLAY --> DRM[drm]
    DISPLAY --> VGPU[virtio_gpu]
    INPUT --> VIHID[virtio_input + i2c_hid]
```

## Alignment with `folder_structure.md`

| Target bucket (folder_structure) | Current paths present | Alignment | Notes |
| --- | --- | --- | --- |
| `core/drivers/bus/` | `core/drivers/bus/{i2c,spi,usb,gpio,cluster_bus}` | Strong | Good base layout. |
| `core/drivers/net/` | `core/drivers/net/{core,virtio_net}` | Partial | Shared network-driver class contract is now present; physical NIC coverage still pending. |
| `core/drivers/block/` + `core/drivers/storage/` | both exist (`block/virtio_blk`, `storage/nvme`) | Partial | Overlap indicates unresolved taxonomy between block vs storage. |
| `core/drivers/display/` | `core/drivers/display/{drm,virtio_gpu}` | Strong | Matches direction. |
| `core/drivers/input/` | `core/drivers/input/{virtio_input,i2c_hid}` | Strong | Matches direction. |
| `core/drivers/serial/` | multiple UART families | Strong | Good core/arch/vendor spread. |
| `core/drivers/accel/` | present but minimal | Scaffold | Control plane exists; hardware backends limited. |
| `core/drivers/class/*` | `class/{can,sensor,motor,actuator}` | Partial | CAN class now has state/queue/filter contract; hardware backends still limited. |

## Driver status matrix

| Driver domain | Current status | Evidence in tree | Next structural action | Roadmap linkage |
| --- | --- | --- | --- | --- |
| Bus drivers | Partial | `core/drivers/bus/*` populated | Add PCIe host + robust discovery glue for x86/arm64/riscv64. | Phase 1 |
| Network drivers | Partial | `core/drivers/net/virtio_net` | Add physical NIC implementations and capability-safe queue provisioning. | Phase 3 |
| Storage/block drivers | Partial | `core/drivers/block/virtio_blk`, `core/drivers/storage/nvme` | Decide canonical split (`block` frontend vs `storage` transport) and document ownership. | Phase 2, Phase 3 |
| Display drivers | Partial | `core/drivers/display/drm`, `virtio_gpu` | Promote from boot/display to compositor-ready path and memory fencing rules. | Phase 2, Phase 4 |
| Accelerator drivers | Scaffold | `core/drivers/accel/`, `core/drivers/devices/fpga_mgr` | Implement map/unmap lifecycle, IOMMU integration, and URPC queue contracts. | Phase 3 |
| Device classes | Partial | `core/drivers/class/{can,sensor,motor,actuator}` | Ensure class drivers delegate transport specifics to `bus/*` implementations. | Phase 2 |

## Coding tasks identified

1. **Resolve block/storage duplication:** publish a `core/drivers/block` vs `core/drivers/storage` contract and migrate one implementation path to avoid dual registration logic.
2. **Promote discovery stack:** add PCI/ACPI/FDT-aware bus discovery adapters for real hardware in addition to virtio-centric workflows.
3. **Class-driver contract hardening:** extend new net/CAN class contracts to additional classes and enforce bus-agnostic class APIs.
4. **Accelerator maturity:** create common queue, fence, and DMA map APIs under `core/drivers/accel/` to remove per-device ad hoc flows.
