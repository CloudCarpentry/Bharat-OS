# PR Spec: Driver Core Baseline, Device Manager Contract, and Edge I/O Production Slice

## 1. Goal and Non-Goals

### Goals
- Establish a "Driver Core Baseline" for Bharat-OS that enforces a clean separation of mechanism (kernel), hardware control (drivers), and policy/orchestration (services).
- Define the overarching architecture and standard vocabulary for device classes, buses, driver registration, lifecycle events, and power states.
- Prove the architecture by implementing one concrete, production-grade edge I/O slice (GPIO, pinctrl, PWM, LED, gpio-keys).
- Provide a clear, phased roadmap and minimal architecture headers for the USB subsystem without attempting a full implementation yet.
- Establish a device manager contract via documentation and a minimal service skeleton to prove the boundary between hardware control and policy.
- Provide build and compile-time gating tailored to target profiles (e.g., `x86_64` and `arm64` QEMU).

### Non-Goals
- Full implementation of the USB subsystem (xHCI, HID, mass storage, hotplug daemon).
- Becoming a "mini Linux device model" (keep it tailored, small, and structured for Bharat-OS).
- Deep platform rewiring or sweeping kernel core changes outside minimal integration points.
- Full runtime service logic for `devmgr` (only a skeleton proving the boundary is required).

## 2. Exact Folders/Files to Add

### Documentation
- `docs/architecture/pr-spec-driver-baseline.md` (This file)
- `docs/architecture/driver-model-baseline.md`
- `docs/architecture/device-manager-contract.md`
- `docs/architecture/usb-subsystem-roadmap.md`
- `docs/dev/driver-contributor-rules.md`

### Driver Core
- `drivers/core/device_registry.h` / `.c`
- `drivers/core/driver_registry.h` / `.c`
- `drivers/core/match.h` / `.c`
- `drivers/core/event.h` / `.c`
- `drivers/core/power.h` / `.c`
- `drivers/include/driver_core.h` (Centralized class enums, descriptors)

### Edge I/O Production Slice
- `drivers/bus/gpio/gpio_core.h` / `.c`
- `drivers/bus/gpio/pinctrl_core.h` / `.c`
- `drivers/bus/gpio/pwm_core.h` / `.c`
- `drivers/led/led_core.h` / `.c`
- `drivers/input/gpio_keys.h` / `.c`

### USB Baseline
- `drivers/bus/usb/usb_core.h` (Contracts only)

### Service Skeleton
- `services/core/devmgr/devmgr_skeleton.h` / `.c`

### Build and Testing
- CMakeLists.txt modifications for build gating in `drivers/`
- Host/unit tests for driver core lifecycle and Edge I/O components in `tests/drivers/`

## 3. Contract Shapes

- **Device Class Contract**: A single enum representing classes (e.g., `CLASS_GPIO`, `CLASS_LED`, `CLASS_USB_HOST`).
- **Device Descriptor**: Name, class, bus, instance path, IDs, capability/power flags, parent/child relationships, private data pointers.
- **Driver Descriptor**: Name, supported class/bus, match table, `probe()`, `remove()`, `suspend()`, `resume()`, `reset()`, `fault()`.
- **Bus Descriptor**: Register controller, enumerate, register/remove device, rescan, power/hotplug hooks.
- **Event Contract**: Standard events (`ADDED`, `REMOVED`, `CHANGED`, `SUSPEND`, `RESUME`, `FAULT`).

## 4. Build Gating Rules

Compile-time gating using CMake options to allow different profiles (tiny, edge, full):
- `BHARAT_ENABLE_DRIVER_CORE`
- `BHARAT_ENABLE_GPIO`
- `BHARAT_ENABLE_PINCTRL`
- `BHARAT_ENABLE_PWM`
- `BHARAT_ENABLE_LED`
- `BHARAT_ENABLE_INPUT_GPIO_KEYS`
- `BHARAT_ENABLE_USB_CORE_HEADERS`

Target Architectures: Primarily tested and gated for `x86_64` (QEMU) and `arm64` (QEMU virt).

## 5. Test Expectations

- **Host/Unit Tests**: For `probe`/`match` logic, invalid-device handling, `remove`/failure cleanup, and event emission.
- **Integration Tests**: Basic wiring tests to prove the Edge I/O components can compile and load their driver descriptors successfully.
- Tests will be integrated with the standard `tools/build.py` workflow.

## 6. Migration Notes

- Existing drivers (if any) are not migrated in this PR.
- This PR sets the structural precedent; future PRs will migrate legacy drivers to the `drivers/` directory using these contracts.
- Kernel changes are minimal; the kernel remains mechanism-focused.

## 7. Implementation Boundaries

### What is Production-Grade
- Driver Core contracts, structs, and registry logic.
- Edge I/O baseline (GPIO, LED, gpio-keys).
- CMake build gating.

### What is Intentionally Stubbed
- `devmgr` service (provides the event reception shape and boundary only).

### What is Deferred to Follow-up PRs
- USB xHCI, HID, Mass Storage, Networking.
- Advanced devmgr policy enforcement and hotplug daemon.
- Migrating legacy hardware control out of the kernel.