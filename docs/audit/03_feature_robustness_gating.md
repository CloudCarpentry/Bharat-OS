# Feature Robustness & Gating Analysis

In order to ensure that Bharat-OS remains a composable platform, we need to guarantee that features are safely gated behind configuration flags and do not leak across environments when disabled.

## 1. Feature Flag Sprawl
Currently, there are over 600 `#ifdef` checks and 200 `#ifndef` checks in the core layer, with 260+ using the `BHARAT_ENABLE_` prefix. While conditional compilation is necessary for an OS, unchecked feature flags can result in dead code or untested combinations (the "ifdef hell").

## 2. Hardcoded Fallbacks
The system employs numerous silent "fallbacks", where if a hardware feature or dynamic configuration fails, a hardcoded default is used instead of failing cleanly or propagating the error.

*   **Storage Fallbacks**: E.g., `g_system_device_id = 42` used in `core/services/system/filesystem/main.c`. If an invalid storage mount occurs, falling back to a dummy device ID masks the failure and can cause silent data loss or startup failure later.
*   **Virtual Queues**: E.g., `core/drivers/input/virtio_input/virtio_input.c:329` has a "Software queue fallback" which can silently impact performance predictability if the hypervisor queue is misconfigured.

## 3. Lack of Unified `build_config.json` Mapping
While there is a `build_config.json` that drives the build, there appears to be drift between what the build system orchestrates and the macros checked inside the C code.

## Actionable Recommendations to Improve Robustness

1.  **Fail-Fast by Default**: Remove silent fallbacks like hardcoded dummy device IDs. If an essential system component (like the root file system or boot device) is missing, the system should cleanly panic or enter a recovery shell, not pretend everything is fine.
2.  **Centralized Feature Manifest**: Implement a macro checking script in `tools/lint/` that validates all `#ifdef BHARAT_ENABLE_` macros against a known manifest of valid configuration flags, preventing typos or dead flags.
3.  **Telemetry on Fallbacks**: If a fallback (like software rendering or polled I/O instead of interrupts) *must* be used, it must emit a critical telemetry event. The telemetry manager currently has hooks for `cpu_fallback_count` (in `accelmgr_broker.c`), but this pattern needs to be applied to all hardware drivers.
