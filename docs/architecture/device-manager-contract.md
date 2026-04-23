# Device Manager (devmgr) Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## Purpose
This document defines the role and boundary of the Device Manager (`devmgr`) in Bharat-OS. The `devmgr` acts as the service-level policy orchestrator for hardware devices, acting entirely outside of kernel mechanism and driver control paths.

## Roles and Responsibilities
- **Inventory**: Maintain an authoritative user-space view of all registered devices across all buses.
- **Hotplug**: Receive and process hotplug events (`ADDED`, `REMOVED`) emitted by the driver core.
- **Policy**: Enforce access control, profile-based enablement, and dynamic routing (e.g., storage, input).
- **Permissions**: Gate device node access to other user-space services.
- **Power Requests**: Orchestrate system-wide or device-specific suspend/resume commands and route them to drivers.

## Architecture Boundary
The driver framework provides events (state changes) to the `devmgr`. The `devmgr` never directly touches hardware registers, maps device memory, or interacts with IRQs. It purely consumes standard descriptors and events, and dispatches configuration or policy commands back down to the framework.
