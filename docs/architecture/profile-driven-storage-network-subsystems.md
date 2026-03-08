# Profile-Driven Storage and Network Subsystems

This note defines how Bharat-OS selects storage and networking capabilities from:

1. Device profile (desktop/mobile/edge/datacenter/network appliance/RTOS/automotive)
2. Personality profile (Linux/Windows/Mac/native)
3. Architecture family (x86/ARM/RISC-V)
4. Runtime boot hardware profile (`generic`, `vm`, `mobile`, `network_appliance`, etc.)

## Storage capability matrix

The core block layer remains always available, then profile-specific drivers are toggled:

- NVMe: preferred for datacenter, desktop, and network appliances
- AHCI/SATA: enabled by default for x86 targets
- eMMC/SD: mobile/edge/RTOS class systems
- flash/MTD: mobile/edge/robotics/automotive-ECU style systems
- RAM disk: always available for early boot and recovery

## Network capability matrix

The stack starts from Ethernet baseline and selects either:

- lightweight embedded stack (edge/RTOS-like systems), or
- full TCP/IP stack (desktop/datacenter/network appliance and rich personalities)

Additional options:

- zero-copy packet path (datacenter and network appliance)
- virtio-net first-class support for VM/datacenter paths
- Wi-Fi enabled for mobile and infotainment style profiles
- TSN/CAN/EtherCAT extension bits for automotive ECU and RTOS profiles

## Runtime override model

`bharat_subsystems_init(boot_hw_profile)` applies runtime tuning on top of compile-time defaults.

Examples:

- `vm`: prefer virtio-net + full TCP/IP, de-emphasize AHCI
- `mobile`: enforce eMMC/SD + flash/MTD + Wi-Fi
- `network_appliance`: enforce Ethernet + zero-copy path and disable Wi-Fi

This keeps the core generic while still adapting boot behavior to hardware role.

## Default-driver and userspace/personality contract

- Default driver registration is now profile-gated in `device_register_builtin_drivers()`: drivers are only registered when their subsystem feature bit is active (for example NVMe, AHCI, Wi-Fi, virtio-net).
- If subsystem policy has not been initialized yet, driver registration falls back to `generic` profile defaults to keep legacy boot and tests stable.
- A userspace SDK API (`bharat_get_subsystem_caps`) is provided as a stable contract point for personalities and libraries; it currently returns `-ENOSYS` until the syscall path is wired, but this avoids ABI churn later.
