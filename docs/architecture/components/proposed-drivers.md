---
title: Proposed Drivers Architecture and Roadmap (Aligned to Current Tree)
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - components
see_also:
  - README.md
---
# Proposed Drivers Architecture and Roadmap (Aligned to Current Tree)

This document tracks **what exists now** and the production-grade first slice implemented for network and CAN driver classes.

## What already exists in tree (baseline)

- Bus foundations: `core/drivers/bus/{i2c,spi,usb,gpio,cluster_bus}`.
- Network baseline: `core/drivers/net/virtio_net` plus generic class contract in `core/drivers/net/core`.
- Storage baseline: `core/drivers/storage/nvme` and `core/drivers/block/virtio_blk`.
- Display baseline: `core/drivers/display/{drm,virtio_gpu}`.
- Input baseline: `core/drivers/input/{virtio_input,i2c_hid}`.
- Serial breadth: `core/drivers/serial/{ns16550,pl011,dw_apb_uart,imx_lpuart,cadence_uart,sifive_uart}`.
- Class and device examples: `core/drivers/class/{can,sensor,motor,actuator}`, `core/drivers/devices/{virt_can,can_loopback,fpga_mgr,ptp_clock}`.

## Networking + CAN first-slice status

### Implemented in this slice

1. **Common network class contract (`core/drivers/include/core/drivers/net/net_driver.h`)**
   - Device descriptor with MAC, MTU, queue counts, carrier state, and feature flags.
   - Ops table: `probe/init/start/stop/tx/rx/poll/irq` + MTU/promisc controls.
   - Common stats counters and explicit lifecycle state.
   - Registry helpers in `core/drivers/net/core/net_driver_core.c`.

2. **`core/drivers/net/` baseline hardening**
   - `virtio_net` migrated to use the common contract.
   - TX path now flows through queue-aware generic helpers.
   - RX queue + poll fallback path available for QEMU-friendly mock RX tests.
   - Carrier and MTU handling are fail-closed; invalid MTU returns an error.

3. **`core/drivers/class/can/` production baseline**
   - Controller contract extended with capabilities, bitrate bounds, loopback/listen-only toggles, poll/irq hooks.
   - State machine helpers: stopped/error-active/error-passive/bus-off/recovering.
   - Queue-backed TX/RX plumbing and acceptance-filtered RX delivery.
   - Error/statistics updates for bus-off, overruns, drops, and error-frame accounting.

4. **Profile-aware build stance**
   - CAN core remains policy-controlled through `cmake/modules/BharatComponentPolicy.cmake`.
   - Automotive profiles continue forcing CAN core + loopback on.
   - Desktop/headless targets keep generic NIC (`virtio-net`) path available.

### Deliberately deferred

- Physical NIC families (ixgbe/i40e/mlx5/ENETC).
- Hardware-backed CAN FD controller drivers.
- Service-level CAN orchestration (`core/services/network/can/*`) beyond existing `core/services/can/*` primitives.
- Full IRQ-driven data path for all NIC backends (poll fallback remains required).

## Validation scope guidance

- **Build coverage:** `x86_64_desktop_headless`, `arm64_desktop_headless`, `riscv64_desktop_headless`.
- **Runtime truthfulness:** if QEMU target lacks real CAN hardware, validate with loopback/controller tests only and do not claim hardware parity.
- **Host coverage:** contract/state/queue/filter tests in `quality/tests/host/drivers` and `quality/tests/host/services`.

## Updated proposal by domain

### 1) Bus and interconnect (`core/drivers/bus/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| PCIe host bridge + full enumeration | Baseline scaffold now in `core/drivers/bus/pcie` (enumeration TODO) | High |
| I3C support | Missing | Medium |
| USB host hardening (xHCI/DWC3 profiles) | Generic USB folder exists | Medium |

### 2) Network (`core/drivers/net/`, `core/drivers/class/can/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| Common net class contract + virtio baseline | Landed (`core/drivers/include/core/drivers/net/net_driver.h`, `core/drivers/net/core`, `virtio_net`) | High |
| CAN core state/queue/filter contract | Landed (`core/drivers/class/can/can_controller_core.c`) | High |
| Physical NIC families + real CAN FD | Missing | High |
| Wi-Fi 6 framework | Missing | Medium |

### 3) Storage (`core/drivers/storage/`, `core/drivers/block/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| NVMe production path | `core/drivers/storage/nvme` exists | High (maturity) |
| UFS + eMMC host stacks | Missing | Medium |
| SPI NOR + recovery integration | Missing | Medium |

## Integration rules (unchanged)

1. Drivers must consume HAL contracts rather than architecture-private register logic.
2. Device MMIO/IRQ/DMA authority must be capability-validated through manager paths.
3. High-throughput paths should prefer URPC/shared-memory interfaces over synchronous RPC for data plane.
4. Driver enablement should remain profile-gated through build policy toggles.
