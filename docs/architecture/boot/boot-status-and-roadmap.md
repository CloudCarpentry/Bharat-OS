# Boot Architecture Status and Roadmap

## Current Code Status (March 2026)

This document summarizes the current implementation status and roadmap for the Bharat-OS boot architecture. The maturity taxonomy used aligns with the canonical definitions: **Scaffold**, **Partial**, **Baseline**, and **Production**.

### Kernel & Core Boot
*   **Kernel Early Boot (`core/kernel/main.c`)**: Baseline. The thin orchestrator receives normalized boot info, initializes CPU-local early state, handles BSP/AP split, and calls staged common boot functions.
*   **Boot Phase Orchestration**: Baseline. The BSP boot flow is phase-based (early boot -> security/bootstrap setup -> memory setup -> core/platform/runtime services setup -> runtime entry selection).
*   **Boot Contracts (`boot_info_t`)**: Baseline. Standardization on `boot_info_t` under `boot/include/boot/` is complete. Adapters for Multiboot2, FDT, and OpenSBI normalize hardware-specific properties into this structure prior to early validation.

### Initialization & Service Launch
*   **Init Service (`init`)**: Scaffold. Bootstrap logs exist, and startup graph actions are currently in a TODO/scaffold state.
*   **Service Manager (`servicemgr`)**: Scaffold. Init path exists, but the event loop is a placeholder.

### Display & Console
*   **Console (`console`)**: Scaffold. Features an infinite loop and placeholder for URPC routing.
*   **Boot Display Daemon (`boot_displayd`)**: Partial. Framebuffer rectangle helpers and mocked early UI flow are implemented.

### Memory Configuration during Boot
*   **Per-Core Memory State**: Partial. Address space structures exist but lack cross-core usage via capability/message.
*   **Memory Models (MMU/MMU-lite/MPU)**: Scaffold/Partial. The unified memory authority path is defined conceptually but requires complete backend-driven plumbing for each profile.

## Roadmap & Future Work

The following features are planned for future development to harden and expand the boot phase:

1.  **Staged Boot Health and Self-Test Policy**
    *   Implement and enforce policy classes (mandatory, quick, extended, manufacturing, benchmark-only) per profile.
2.  **Console Policy Framework**
    *   Transition from early-boot polling console to interrupt-driven runtime console.
    *   Implement minimal kernel monitor/shell for diagnostic fallback.
3.  **Init and Service Handoff**
    *   Flesh out the `sysmgr` / `init` service to read profile/hardware state, query the subsystem registry, start services, and enforce profile-based policies. This must replace direct kernel-side app launches.
4.  **Failure Routing and Recovery Policy**
    *   Introduce explicit recovery modes and crash containment loops (`faultmgr`).
5.  **Steady-State Runtime Ownership Cleanup**
    *   Completely remove `LEGACY_BRINGUP` mode once all test and demo behaviors are fully isolated into dedicated session profiles.
6.  **Secure and Measured Boot**
    *   Flesh out secure boot attestation and add secure update/rollback control points into the boot chain.

*Note: For the broader project code status spanning other subsystems, refer to `docs/dev/current-code-status.md`.*
