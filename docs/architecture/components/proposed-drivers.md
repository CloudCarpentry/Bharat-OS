# Proposed Drivers Architecture and Roadmap (Aligned to Current Tree)

This document updates the proposed driver roadmap to reflect what already exists in `drivers/` and what remains to be implemented.

## What already exists in tree (baseline)

- Bus foundations: `drivers/bus/{i2c,spi,usb,gpio,cluster_bus}`.
- Network baseline: `drivers/net/virtio_net`.
- Storage baseline: `drivers/storage/nvme` and `drivers/block/virtio_blk`.
- Display baseline: `drivers/display/{drm,virtio_gpu}`.
- Input baseline: `drivers/input/{virtio_input,i2c_hid}`.
- Serial breadth: `drivers/serial/{ns16550,pl011,dw_apb_uart,imx_lpuart,cadence_uart,sifive_uart}`.
- Class and device examples: `drivers/class/{can,sensor,motor,actuator}`, `drivers/devices/{virt_can,can_loopback,fpga_mgr,ptp_clock}`.

## Updated proposal by domain

### 1) Bus and interconnect (`drivers/bus/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| PCIe host bridge + full enumeration | Not explicit in current tree | High |
| I3C support | Missing | Medium |
| USB host hardening (xHCI/DWC3 profiles) | Generic USB folder exists | Medium |

### 2) Network (`drivers/net/`, `drivers/class/can/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| Physical NIC families (ixgbe/i40e/mlx5/ENETC) | Missing in tree | High |
| CAN FD production drivers | Class and virtual CAN examples exist | High |
| Wi-Fi 6 framework | Missing | Medium |

### 3) Storage (`drivers/storage/`, `drivers/block/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| NVMe production path | `drivers/storage/nvme` exists | High (maturity) |
| UFS + eMMC host stacks | Missing | Medium |
| SPI NOR + recovery integration | Missing | Medium |

### 4) Display/graphics (`drivers/display/`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| DRM/KMS composition path | `drivers/display/drm` exists | High (feature depth) |
| virtio-gpu bring-up quality | `drivers/display/virtio_gpu` exists | Medium |
| DSI/DCSS/mobile controllers | Missing | Medium |

### 5) Accelerators (`drivers/accel/`, `drivers/devices/fpga_mgr`)

| Proposal | Current baseline | Priority |
| --- | --- | --- |
| Common accel queue + fence framework | Minimal accel tree | High |
| NVDLA / Edge TPU / QAT integration | Missing | Medium |
| FPGA runtime reconfiguration maturity | `drivers/devices/fpga_mgr` exists | Medium |

## Integration rules (unchanged)

1. Drivers must consume HAL contracts rather than architecture-private register logic.
2. Device MMIO/IRQ/DMA authority must be capability-validated through manager paths.
3. High-throughput paths should prefer URPC/shared-memory interfaces over synchronous RPC for data plane.
4. Driver enablement should remain profile-gated through build policy toggles.

## Coding tasks identified

1. **Real hardware NIC backlog:** implement at least one production NIC family for non-virtio environments.
2. **Storage taxonomy cleanup:** remove ambiguity between `drivers/block` and `drivers/storage` with a documented split.
3. **CAN production readiness:** graduate from loopback/virtual examples to hardware-backed CAN FD implementations.
4. **Accelerator framework bring-up:** introduce shared ring/fence APIs to support multiple accelerator backends consistently.
5. **Driver CI matrix expansion:** add per-driver smoke tests across qemu-virt x86_64/arm64/riscv64 targets.
