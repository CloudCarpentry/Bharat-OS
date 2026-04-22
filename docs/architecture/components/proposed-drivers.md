# Proposed Drivers Architecture and Roadmap (Aligned to Current Tree)

This document tracks **what exists now** and the production-grade first slice implemented for network and CAN driver classes.

## What already exists in tree (baseline)

- Bus foundations: `drivers/bus/{i2c,spi,usb,gpio,cluster_bus}`.
- Network baseline: `drivers/net/virtio_net` plus generic class contract in `drivers/net/core`.
- Storage baseline: `drivers/storage/nvme` and `drivers/block/virtio_blk`.
- Display baseline: `drivers/display/{drm,virtio_gpu}`.
- Input baseline: `drivers/input/{virtio_input,i2c_hid}`.
- Serial breadth: `drivers/serial/{ns16550,pl011,dw_apb_uart,imx_lpuart,cadence_uart,sifive_uart}`.
- Class and device examples: `drivers/class/{can,sensor,motor,actuator}`, `drivers/devices/{virt_can,can_loopback,fpga_mgr,ptp_clock}`.

## Networking + CAN first-slice status

### Implemented in this slice

1. **Common network class contract (`drivers/include/drivers/net/net_driver.h`)**
   - Device descriptor with MAC, MTU, queue counts, carrier state, and feature flags.
   - Ops table: `probe/init/start/stop/tx/rx/poll/irq` + MTU/promisc controls.
   - Common stats counters and explicit lifecycle state.
   - Registry helpers in `drivers/net/core/net_driver_core.c`.

2. **`drivers/net/` baseline hardening**
   - `virtio_net` migrated to use the common contract.
   - TX path now flows through queue-aware generic helpers.
   - RX queue + poll fallback path available for QEMU-friendly mock RX tests.
   - Carrier and MTU handling are fail-closed; invalid MTU returns an error.

3. **`drivers/class/can/` production baseline**
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
- Service-level CAN orchestration (`services/network/can/*`) beyond existing `services/can/*` primitives.
- Full IRQ-driven data path for all NIC backends (poll fallback remains required).

## Validation scope guidance

- **Build coverage:** `x86_64_desktop_headless`, `arm64_desktop_headless`, `riscv64_desktop_headless`.
- **Runtime truthfulness:** if QEMU target lacks real CAN hardware, validate with loopback/controller tests only and do not claim hardware parity.
- **Host coverage:** contract/state/queue/filter tests in `tests/host/drivers` and `tests/host/services`.

## Updated proposal by domain

### 1) Bus and interconnect (`drivers/bus/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| PCIe host bridge + full enumeration | Baseline scaffold now in `drivers/bus/pcie` (enumeration TODO) | High |
| I3C support | Missing | Medium |
| USB host hardening (xHCI/DWC3 profiles) | Generic USB folder exists | Medium |

### 2) Network (`drivers/net/`, `drivers/class/can/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| Common net class contract + virtio baseline | Landed (`drivers/include/drivers/net/net_driver.h`, `drivers/net/core`, `virtio_net`) | High |
| CAN core state/queue/filter contract | Landed (`drivers/class/can/can_controller_core.c`) | High |
| Physical NIC families + real CAN FD | Missing | High |
| Wi-Fi 6 framework | Missing | Medium |

### 3) Storage (`drivers/storage/`, `drivers/block/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| NVMe production path | `drivers/storage/nvme` exists | High (maturity) |
| UFS + eMMC host stacks | Missing | Medium |
| SPI NOR + recovery integration | Missing | Medium |

## Integration rules (unchanged)

1. Drivers must consume HAL contracts rather than architecture-private register logic.
2. Device MMIO/IRQ/DMA authority must be capability-validated through manager paths.
3. High-throughput paths should prefer URPC/shared-memory interfaces over synchronous RPC for data plane.
4. Driver enablement should remain profile-gated through build policy toggles.
