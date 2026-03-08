# Device Profiles

Bharat-OS supports multiple interaction models and required subsystems based on the specific device class. Instead of forcing a one-size-fits-all binary, we define these device profiles. Each profile establishes the base functionality required, ensuring optimal performance for its given domain.

## Hardware MMU Quirks & Capabilities

The following capabilities are mapped via `mmu_ops_t` and `boards.json` to configure the hardware memory management unit appropriately for each profile:
* **`mmu_mode`**: Selects the specific translation format (e.g., `4_level_or_la57` for x86, `vmsav8_64` for ARM64, `sv39` for RISC-V).
* **`granule`**: The baseline page size (usually `4k`, but ARM64 can support `16k` or `64k`).
* **`has_iommu`**: Indicates whether an IOMMU is available for DMA isolation.
* **`quirks`**: Board-specific deviations. Common quirks include:
  * `coherent_dma`: Indicates whether the DMA controller is cache-coherent (if missing, software cache maintenance is required).
  * `svpbmt_optional`: For RISC-V, indicates whether Page-Based Memory Types might be available.
  * `rtos_strict`: Forces the OS to disable demand paging and pre-allocate all memory to guarantee deterministic latency.
  * `no_mmu`: Indicates the device only has an MPU (e.g. Cortex-M).

## Headless Edge Device (`HEADLESS_EDGE_DEVICE`)
Typically applies to network routers, gateways, server-like appliances, and remote compute nodes.

* **Required Subsystems:** Boot/platform init, memory manager, scheduler, basic IPC, networking stack, remote management.
* **Optional Subsystems:** Serial/web admin panel, front-panel LED state tracking.
* **Rendering Path:** No local GUI. Text/serial console only.
* **Input Model:** Remote SSH/management API, serial console.
* **Power/Thermal:** Often wired power, minimal active thermal management.
* **Accelerators:** Network DPDK/crypto accelerators if available.
* **Storage:** Basic configuration store.

## Micro Display Device (`MICRO_DISPLAY_DEVICE`)
Typically applies to small status units, drones controllers, handheld tools, or early-bring-up boards.

* **Required Subsystems:** Boot, memory, timer, basic scheduler, framebuffer driver, tiny widget toolkit, GPIO/touch input, power manager.
* **Optional Subsystems:** Battery/charging hooks, offline diagnostic screen, buzzer/vibration.
* **Rendering Path:** Framebuffer only. Direct-to-buffer rendering or minimal retained mode.
* **Input Model:** GPIO buttons, simple touch overlays.
* **Power/Thermal:** Strict power management, battery awareness.
* **Accelerators:** Minimal (possibly basic 2D blitters/DMAs).
* **Storage:** Local flash/EEPROM for configurations and OTA.

## Panel HMI Device (`PANEL_HMI_DEVICE`)
Typically applies to industrial control panels, medical monitors, home appliance screens, or robotics status displays.

* **Required Subsystems:** Framebuffer, input/touch core, embedded UI toolkit, secure kiosk shell, watchdog, local storage.
* **Optional Subsystems:** Multilingual text rendering, audio alerts.
* **Rendering Path:** Framebuffer with lightweight 2D software-rendered UI (e.g., labels, buttons, simple windows).
* **Input Model:** Multi-touch screens, keypads.
* **Power/Thermal:** Can be battery or wired, requires solid thermal profiling.
* **Accelerators:** 2D display engines, DMA blitters, simple display controllers.
* **Storage:** Local persistent storage for logs and configuration.

## Robot Control Device (`ROBOT_CONTROL_DEVICE`)
Focuses on low latency and real-time operational state tracking.

* **Required Subsystems:** Framebuffer, sensor bus framework, robotics control framework, URPC IPC.
* **Optional Subsystems:** Video/camera overlays, teleoperation widgets.
* **Rendering Path:** Framebuffer for local diagnostics, potentially an accelerated compositor if rich HMI is required.
* **Input Model:** Custom joysticks/controllers, touch panels.
* **Power/Thermal:** High demand, requires active governor.
* **Accelerators:** DSP/NPU for perception tasks, hardware encoders for video streams.

## Drone Control Device (`DRONE_CONTROL_DEVICE`)
Prioritizes mission-critical telemetry, fail-safe rendering, and real-time execution.

* **Required Subsystems:** Tiny framebuffer driver, text/status renderer, low-power display updates, flight subsystem.
* **Optional Subsystems:** FPV/video output abstraction.
* **Rendering Path:** Minimal framebuffer, text renderer.
* **Input Model:** RC receivers, GPIO switches.
* **Power/Thermal:** Extreme power constraints, weight/thermal limits.
* **Accelerators:** Sensor coprocessors, ISP for cameras.

## Network Appliance Device (`NETWORK_APPLIANCE_DEVICE`)
A specialized variant of the headless edge, often found in datacenters or comms closets.

* **Required Subsystems:** Network dataplane subsystem, high-speed driver models.
* **Rendering Path:** LCD front panel (character display) or text console.
* **Input Model:** Simple membrane keys on the chassis, remote interfaces.
* **Accelerators:** Crypto offload, SmartNICs, packet processing DPUs.

## Desktop Workstation (`DESKTOP_WORKSTATION`)
Full user environments with heavy multimedia requirements.

* **Required Subsystems:** Full compositor, advanced IPC, memory tiering/demand paging.
* **Rendering Path:** GPU modesetting, accelerated compositor window manager. Framebuffer used only for fallback/boot.
* **Input Model:** Complex USB HID (mouse, keyboard, specialized peripherals).
* **Power/Thermal:** High consumption, ACPI standard support.
* **Accelerators:** Full desktop GPUs.

## Datacenter Node (`DATACENTER_NODE`)
High-density compute focusing on throughput, isolation, and orchestration.

* **Required Subsystems:** NUMA-aware memory manager, virtualization hooks, AI-driven scheduler.
* **Rendering Path:** Headless / remote BMC only.
* **Input Model:** IPMI, remote serial.
* **Power/Thermal:** Rack-level thermal management.
* **Accelerators:** Heavy reliance on datacenter GPUs and NPUs for ML workloads.

## Automotive Safety RT (`BHARAT_PROFILE_AUTOMOTIVE_SAFETY_RT`)
Used for the highest-criticality domain (watchdog and fail-safe logic).

* **Required Subsystems:** Minimal boot path, hard RT scheduler class, watchdog chain, health monitor, emergency state transitions.
* **Optional Subsystems:** Limited diagnostics output.
* **Rendering Path:** None (headless).
* **Input Model:** Sensor/health buses and supervisory interrupts.
* **Power/Thermal:** Must remain operational across degraded conditions.
* **Accelerators:** Typically none; prefers deterministic CPU execution.

## Automotive Control (`BHARAT_PROFILE_AUTOMOTIVE_CONTROL`)
Used for motion/powertrain and battery control domains.

* **Required Subsystems:** Hard RT scheduler, timer subsystem, ADC/PWM/control drivers, control IPC, deterministic logging.
* **Optional Subsystems:** Sensor fusion helpers and calibration services.
* **Rendering Path:** None or minimal debug console.
* **Input Model:** Real-time sensor feeds and control bus inputs.
* **Power/Thermal:** Strict deterministic behavior over throughput.
* **Accelerators:** Optional DSP support if bounded-latency integration is available.

## Automotive Gateway (`BHARAT_PROFILE_AUTOMOTIVE_GATEWAY`)
Used for ECU bridging and diagnostics routing.

* **Required Subsystems:** CAN/CAN-FD, LIN, Ethernet/TSN stack, SOME/IP services, diagnostics and security policy enforcement.
* **Optional Subsystems:** Telemetry buffering and fleet connectivity agents.
* **Rendering Path:** Headless.
* **Input Model:** Multi-bus packet and service traffic.
* **Power/Thermal:** Continuous operation, high reliability.
* **Accelerators:** Crypto offload and network acceleration if available.

## Automotive AI Edge (`BHARAT_PROFILE_AUTOMOTIVE_AI_EDGE`)
Used for central compute nodes running Linux companion workloads.

* **Required Subsystems:** Companion-mode IPC, service runtime, telemetry/diagnostics bridge, secure update client.
* **Optional Subsystems:** AI accelerator orchestration and multimedia services.
* **Rendering Path:** Linux-managed HMI/infotainment path.
* **Input Model:** Sensor fusion streams, cloud/connectivity endpoints.
* **Power/Thermal:** Requires runtime power and thermal policy coordination.
* **Accelerators:** NPU/GPU for perception and AI inference.
