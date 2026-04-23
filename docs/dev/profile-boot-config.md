# Developer Reference: Profile Boot Configuration

## Overview

This guide explains how developers configure, select, and interact with the Bharat-OS profile-driven boot lifecycle. It details how the build system maps a `Device Profile` to an execution environment, how services register themselves, and how to verify the runtime state.

## 1. How the Profile is Selected

The profile is determined at build time through CMake parameters. The build scripts (`./build.sh` or `.\build.ps1`) combine:

1.  **Architecture:** E.g., `--arch arm64` or `-Arch riscv64`.
2.  **Board Configuration:** The specific hardware layout.
3.  **Device Profile:** E.g., `DESKTOP`, `NETWORK_APPLIANCE`, `DRONE`.
4.  **Kernel Execution Profile:** E.g., `GP` (General Purpose), `RT` (Real-Time), `MIX`.

This results in a combined `bharat_subsystem_profile_t` structure embedded in the compiled kernel and accessible via `kernel/include/profile/profile.h`.

### Example Build Output

During configuration, you will see output like this:

```
-- Device Profile: DESKTOP
-- Kernel Execution Profile: GP
-- Personality: FULL
```

This ensures the profile resolution is concrete and validates component policy resolution during build/configuration.

## 2. Passing Board, Profile, and Personality

The kernel expects these values to be available during the very first boot phase. They are typically encoded into a configuration block (like a Device Tree, ACPI table, or a static C structure for tiny embedded profiles) and passed into `bharat_subsystems_init(const char *boot_hw_profile)`.

The System Policy Manager (`sysmgr`) queries these parameters via the `bharat_boot_active_policy()` (or similar) API to determine its operating mode.

## 3. Registering a New Service

When you write a new service, you **must not** modify the kernel's `main()` or scattered `#ifdef` blocks. Instead, your service must be registered in the Subsystem Registry (`kernel/include/subsystem_profile.h` or equivalent domain registry).

### Service Descriptor

A service descriptor defines its requirements. It typically looks like this conceptually:

```c
typedef struct {
    const char *name;
    uint32_t type;               // e.g., SERVICE_TYPE_TELEMETRY
    uint32_t init_priority;      // Lower is earlier
    uint32_t required_caps;      // e.g., CAP_NET | CAP_STORAGE
    uint32_t supported_profiles; // e.g., PROFILE_GP | PROFILE_RT
    uint32_t restart_policy;     // e.g., RESTART_ISOLATE
    void (*entry_point)(void);
} bharat_service_descriptor_t;
```

### Registration Macro

You typically register your service using a linker-section macro:

```c
BHARAT_REGISTER_SERVICE(my_telemetry_service, {
    .name = "telemetry_mgr",
    .type = SERVICE_TYPE_TELEMETRY,
    .init_priority = 50,
    .supported_profiles = PROFILE_GP | PROFILE_CLOUD,
    .entry_point = my_telemetry_main
});
```

This allows the `sysmgr` to iterate over all registered services, check the `supported_profiles` against the currently active profile, and selectively start them.

## 4. Declaring Supported Profiles

A service must explicitly declare which profiles it supports using a bitmask or array of enums.

*   **Rule:** If a service does *not* declare support for the current profile (e.g., a Media Manager trying to start on a `TINY_EMBEDDED` profile), `sysmgr` will ignore it.
*   **Rule:** If a service requires capabilities (like `CAP_NET`) that the current profile disables (e.g., `SAFETY` profile disables full networking), `sysmgr` will reject its registration.

## 5. Testing the Boot Path

It is mandatory to write end-to-end tests that verify the boot path *for each profile*. A successful boot is not just reaching a shell; it is the correct steady-state configuration of `sysmgr` and its children.

### Test Matrix

Your CI or hardware test scripts should verify the following conditions:

1.  **Steady State:** After the "BOOT COMPLETE" log message (or equivalent signal), the `sysmgr` is active and running.
2.  **Service Filtering (Positive):** For a `DESKTOP` build, ensure `ui_mgr` and `netstack` are running.
3.  **Service Filtering (Negative):** For a `DRONE` build, ensure `ui_mgr` and `media_mgr` are *not* running, even if their source code was compiled in.
4.  **Fault Recovery:** Simulate a crash of a restartable service (e.g., send a fatal signal to `telemetry_mgr` on a `CLOUD` profile) and verify `sysmgr` restarts it correctly according to the profile's fault domain policy.
5.  **Critical Failure Escalation:** Simulate a crash of a critical service (e.g., `can_mgr` on an `AUTOMOTIVE` profile) and verify the system transitions to a safe state or reboots.

### Example E2E Validation

E2E testing relies on capturing serial logs or defined memory markers. Avoid fragile timing or GUI checks.

```bash
# Validating Desktop Profile Boot
./build.sh build --target DESKTOP
qemu-system-aarch64 ... | grep -q "sysmgr: started ui_mgr"

# Validating Appliance Profile Boot
./build.sh build --target APPLIANCE
qemu-system-aarch64 ... | grep -q "sysmgr: filtered ui_mgr (unsupported profile)"
```