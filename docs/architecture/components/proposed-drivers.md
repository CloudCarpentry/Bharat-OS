# Proposed Drivers Architecture and Roadmap

This document outlines the potential drivers that can be implemented in the `drivers/` directory, mapped against target architectures (x86_64, arm64, riscv64) and device profiles (Automotive, Drone/Robotics, Edge/Mobile, Datacenter). It aligns with the Bharat-OS capability-based driver boundary model and multikernel messaging architecture.

## 1. Bus and Interconnect Drivers (`drivers/bus/`)

Bus drivers are responsible for hardware discovery, capability enumeration, and IOMMU group assignment.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **PCIe Host Bridge** | Comprehensive PCIe enumeration, MSI/MSI-X setup, and PCIe capability management. | x86_64, arm64, riscv64 | Cloud, Edge, Automotive |
| **I2C/SPI Generic Core** | Master/Slave controller abstractions for low-speed peripherals and sensors. | arm64, riscv64 | Drone, Edge, Automotive |
| **CXL (Compute Express Link)** | High-speed cache-coherent interconnect for memory expansion and accelerators. | x86_64, arm64 | Cloud, Datacenter |
| **MIPI I3C** | Next-generation sensor bus, backwards compatible with I2C, used in modern mobile and IoT. | arm64, riscv64 | Mobile, Edge |
| **USB xHCI / DWC3** | Universal Serial Bus 3.0+ host and dual-role controller drivers. | x86_64, arm64, riscv64 | Desktop, Edge, Automotive |

## 2. Network Drivers (`drivers/net/` & `drivers/class/can/`)

Network drivers operate in user-space isolated domains, forwarding packets to the `netstack` via lockless shared memory (URPC).

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **Intel ixgbe / i40e** | 10GbE / 40GbE server NIC drivers with SR-IOV support. | x86_64 | Cloud, Datacenter |
| **Mellanox ConnectX (mlx5)** | High-throughput NIC driver with RDMA (RoCE) hardware offload support. | x86_64, arm64 | Cloud, Datacenter |
| **NXP ENETC** | Ethernet controller common in NXP automotive and edge SoCs (e.g., i.MX8). | arm64 | Automotive, Edge |
| **NVIDIA Orin Ethernet** | 10GbE MAC designed for autonomous driving platforms. | arm64 | Automotive, Robotics |
| **Bosch M_CAN** | Native Controller Area Network (CAN FD) driver for ECU communications. | arm64, riscv64 | Automotive, Drone |
| **802.11ax / Wi-Fi 6 (Generic)** | PCIe-based wireless network controller framework. | x86_64, arm64 | Mobile, Edge, Drone |

## 3. Storage Drivers (`drivers/storage/`)

Storage drivers manage block devices and non-volatile memory, serving the `file_system` subsystem.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **NVMe (PCIe)** | High-performance Non-Volatile Memory Express block driver. | x86_64, arm64, riscv64 | Cloud, Desktop, Edge |
| **UFS (Universal Flash Storage)** | High-speed serial interface for embedded flash storage (replaces eMMC). | arm64 | Mobile, Automotive |
| **eMMC / SDHost** | Secure Digital Host Controller for SD cards and embedded MMC. | arm64, riscv64 | Drone, Edge, Automotive |
| **SPI NOR Flash** | Low-level flash driver for bootloaders, OTA recovery, and littlefs. | arm64, riscv64 | Drone, Edge, Automotive |

## 4. Display and Graphics Drivers (`drivers/display/`)

Display drivers manage framebuffers, display controllers (CRTCs), and command pushing to GPUs.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **DRM/KMS Generic Core** | Direct Rendering Manager and Kernel Mode Setting abstraction framework. | All | Desktop, Automotive |
| **virtio-gpu** | Virtualized GPU driver for QEMU/KVM environments. | x86_64, arm64, riscv64 | Cloud, Development |
| **ARM Mali (Panfrost/Bifrost)** | Open-source driver approach for ARM Mali GPUs. | arm64 | Mobile, Automotive |
| **NXP DCSS / LCDIF** | Display controllers for i.MX8 automotive/industrial SoCs. | arm64 | Automotive, Edge |
| **MIPI DSI Host** | Display Serial Interface driver for embedded mobile panels. | arm64, riscv64 | Mobile, Automotive |

## 5. Accelerator Drivers (`drivers/accel/`)

Accelerator drivers manage DMA setups, IOMMU mappings, and command queues for NPUs, DSPs, and FPGAs.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **virtio-npu / virtio-accel** | Virtualized Neural Processing Unit interface. | x86_64, arm64 | Cloud, Development |
| **NVIDIA NVDLA** | Open-source Deep Learning Accelerator driver. | arm64, riscv64 | Automotive, Edge, Drone |
| **Google Edge TPU (PCIe/USB)** | Tensor Processing Unit driver for local AI inference. | x86_64, arm64 | Edge, Robotics |
| **Xilinx ZynqMP FPGA Manager**| Driver for reconfiguring programmable logic bitstreams at runtime. | arm64 | Edge, Robotics |
| **Intel QAT** | QuickAssist Technology for cryptographic and compression offload. | x86_64 | Cloud, Datacenter |

## 6. Input and Human Interface Drivers (`drivers/input/`)

Input drivers route human interface events (touch, key, rotary) to the UI subsystem.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **virtio-input** | Virtual keyboard, mouse, and tablet device driver. | x86_64, arm64, riscv64 | Cloud, Desktop |
| **I2C HID** | Human Interface Device protocol over I2C (touchpads, touchscreens). | arm64, x86_64 | Mobile, Desktop |
| **FocalTech / Goodix Touch** | Specific I2C touchscreen controller drivers. | arm64, riscv64 | Mobile, Automotive |
| **Rotary Encoder (GPIO)** | GPIO-based quadrature encoder driver (e.g., for car infotainment knobs). | arm64, riscv64 | Automotive, Edge |

## 7. Sensor Drivers (`drivers/class/sensor/`)

Sensor drivers provide environmental and kinematic data, primarily for the EDGE and DRONE profiles.

| Driver Proposal | Description | Architecture Focus | Primary Profiles |
| :--- | :--- | :--- | :--- |
| **InvenSense MPU6050 / ICM20948** | 6-axis and 9-axis IMU (Accelerometer/Gyroscope) via I2C/SPI. | arm64, riscv64 | Drone, Robotics |
| **Bosch BME280** | Environmental sensor (Temperature, Humidity, Pressure). | arm64, riscv64 | Edge, Drone |
| **LIDAR Generic (UART/SPI)** | Interface for 2D/3D spinning LIDARs (e.g., RPLIDAR). | arm64, x86_64 | Robotics, Automotive |
| **GNSS / GPS (NMEA over UART)** | Serial driver handling GPS location streaming. | arm64, riscv64 | Drone, Automotive |

## Summary of Integration Path

1. **Hardware Abstraction**: All drivers must use the Bharat-OS HAL (`hal_mm`, `hal_interrupt`, `hal_mpa`) rather than raw architecture-specific registers.
2. **Capability Safety**: Device MMIO regions and IRQs must be claimed via `devmgr` and verified through capability tokens before mapping.
3. **Data Path**: Drivers (like Network and Storage) must expose a URPC (shared ring-buffer) interface to subsystems (`netstack`, `file_system`) to minimize synchronous IPC overhead.
4. **CMake Policy**: New drivers are guarded by `BHARAT_ENABLE_DRIVER_*` flags and bound to specific profiles in `cmake/modules/BharatComponentPolicy.cmake`.