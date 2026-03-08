# Device Profiles

Bharat-OS supports multiple interaction models and required subsystems based on the specific device class. Instead of forcing a one-size-fits-all binary, we define these device profiles. Each profile establishes the base functionality required, ensuring optimal performance for its given domain.

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