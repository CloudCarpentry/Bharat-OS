# Init Bootstrap Contract

## Objective
The `init` service in Bharat-OS is the first userspace bootstrap coordinator. It is designed as a small, deterministic, and profile-aware service that limits its logic strictly to bounded dependency checking, profile-filtering, and sequential startup.

## Responsibility Boundaries

### What `init` owns:
- Boot profile selection (e.g., TINY, SMALL, MOBILE, DESKTOP, DRONE, EMBEDDED_RICH).
- Manifest filtering by profile, board, capability, and personality mask.
- Startup sequencing through deterministic, pre-resolved manifest tables.
- Minimal retry and fail-fast behavior based on the required or optional state of the service.
- Compact startup status reporting.
- A stable, minimal handoff boundary to `servicemgr` or future lifecycle managers.

### What `init` does NOT own:
- Long-term crash supervision.
- Process lifecycle management (owned by `process_manager`).
- Address-space lifecycle semantics (owned by `vm_manager`).
- Policy logic related to crypto, devices, power, telemetry, and storage.
- File system parsing or JSON evaluation for dynamic bootstrapping.

## Boot Context Model
The boot sequence reads contextual constraints and parameters encoded in `init_boot_context_t`:
- Profile, Platform ID, Architecture ID, and Board ID.
- Capability Mask to determine supported resources.
- Options like `safe_mode` and `diagnostics_mode`.

## Manifest Format
A static manifest approach drives the initialization. Each build profile compiles in an array of `init_service_desc_t` that sets:
- **Service details**: Function pointers like `start_fn` and `probe_fn`.
- **Filtering masks**: Determines under what circumstances the service starts.
- **Retry behavior**: Limits startup attempts before marking failed.
- **Policy enforcement**: Optional vs. Required (`INIT_SERVICE_REQUIRED` halts boot on failure).

## Profiles Matrix
- `TINY`: Constrained builds, likely only 1 application or basic `namesvc`.
- `SMALL`: Constrained embedded builds with simple service graph and `namesvc`.
- `EMBEDDED_RICH`: Includes `process_manager` and `vm_manager`.
- `MOBILE`: Focuses on network, UI, and display service start.
- `DESKTOP`: Broad service base, extensive diagnostics.
- `DRONE`: Deterministic safety, strict error handling, and low noise logging.

## Hand-off Boundary
`init_handoff_to_supervisor(ctx)` defines the transition seam where `init` halts or idles, transferring long-term monitoring and restarting to full supervisory frameworks.
