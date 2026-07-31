---
title: Surface and Display-Buffer Lifecycle with Direct-Scanout Architecture
status: Baseline
owner: UI Working Group
last_updated: 2026-04-26
tags:
  - docs
  - architecture
  - ui
see_also:
  - docs/architecture/display_subsystem.md
---

# Surface and Display-Buffer Lifecycle with Direct-Scanout Architecture

This document describes the baseline architectural model for capability-safe, multi-buffered, composition-optional user interfaces in Bharat-OS. The subsystem isolates core graphics policies outside of the kernel while strictly enforcing lifecycle and ownership invariants over displays, leases, surfaces, buffers, and fences.

## 1. Component Diagram

The following diagram illustrates the relationship between the client application, the display broker service, the transport-neutral core compositor, and display drivers.

```mermaid
graph TD
    Client[Client Application] -->|IPC / BIDL v2| Broker[Display Broker Service]
    Broker -->|IPC Adapter| Core[Compositor Core]
    Core -->|Reference Backend| Headless[Headless Reference Backend]
    Core -->|Production Backend| DisplayDriver[Hardware Display Driver]
```

---

## 2. State Machines

### A. Surface State-Machine Diagram

Surfaces follow a bounded lifecycle where destruction must wait for scanning buffers to retire.

```mermaid
stateDiagram-v2
    [*] --> FREE
    FREE --> CREATED : bh_compositor_create_surface
    CREATED --> CONFIGURED : bh_compositor_attach_buffer
    CONFIGURED --> VISIBLE : bh_compositor_present
    VISIBLE --> OCCLUDED : occlusion policy
    OCCLUDED --> VISIBLE : visibility policy
    CREATED --> DESTROYED : bh_compositor_destroy_surface (not scanning out)
    CONFIGURED --> DESTROYED : bh_compositor_destroy_surface (not scanning out)
    OCCLUDED --> DESTROYED : bh_compositor_destroy_surface (not scanning out)
    VISIBLE --> DESTROY_PENDING : bh_compositor_destroy_surface (scanning out active)
    DESTROY_PENDING --> DESTROYED : bh_compositor_retire_presentation (last buffer retired)
    DESTROYED --> FREE : Slot recycled, generation incremented
    FREE --> [*]
```

### B. Buffer State-Machine Diagram

Buffers are trackable resource handles that cannot be released while queued or active on display planes.

```mermaid
stateDiagram-v2
    [*] --> FREE
    FREE --> ALLOCATED : bh_compositor_register_buffer (System)
    FREE --> IMPORTED : bh_compositor_register_buffer (Imported)
    ALLOCATED --> ATTACHED : bh_compositor_attach_buffer
    IMPORTED --> ATTACHED : bh_compositor_attach_buffer
    ATTACHED --> QUEUED : bh_compositor_present
    QUEUED --> SCANNING_OUT : acquire fence signals & commit
    SCANNING_OUT --> RETIRED : subsequent frame commits
    RETIRED --> RELEASED : bh_compositor_retire_presentation
    RELEASED --> FREE : bh_compositor_release_buffer, slot recycled
    ALLOCATED --> REVOKED : lease revocation initiated
    IMPORTED --> REVOKED : lease revocation initiated
    ATTACHED --> REVOKED : lease revocation initiated
    QUEUED --> REVOKED : lease revocation initiated
    REVOKED --> RELEASED : bh_compositor_release_lease
```

### C. Lease-Revocation State Machine

Display leases offer exclusive control of output displays. Revocation allows the previous client a short grace period to acknowledge before resource recycling is forced.

```mermaid
stateDiagram-v2
    [*] --> ACTIVE
    ACTIVE --> REVOKING : bh_compositor_revoke_lease (grace period)
    REVOKING --> CLIENT_ACKNOWLEDGED : bh_compositor_acknowledge_lease_revocation
    REVOKING --> DEADLINE_EXPIRED : Clock deadline elapsed
    CLIENT_ACKNOWLEDGED --> REVOKED
    DEADLINE_EXPIRED --> REVOKED
    REVOKED --> SLOT_REUSABLE : bh_compositor_release_lease (resources fully drained)
```

---

## 3. Present Transaction Sequence Diagram

Every presentation undergoes an atomic transactional verification sequence:

```mermaid
sequenceDiagram
    autonumber
    Client->>Display Broker: PresentSurfaceReq (lease_handle, surface_handle, buffer_handle, acquire_fence, deadline)
    Display Broker->>Compositor Core: VALIDATE_CLIENT (owner PID/cap match)
    Display Broker->>Compositor Core: VALIDATE_LEASE (active, has PRESENT right)
    Display Broker->>Compositor Core: VALIDATE_SURFACE (not destroyed, belongs to lease)
    Display Broker->>Compositor Core: VALIDATE_BUFFER (attached to surface, matches constraints)
    Display Broker->>Compositor Core: WAIT_ACQUIRE_FENCE (wait with deadline_ns)
    Compositor Core-->>Display Broker: Fence ready / signaled
    Display Broker->>Compositor Core: EVALUATE_PLANES (direct-scanout vs composition)
    Display Broker->>Compositor Core: COMMIT (update active buffer, transition old buffer)
    Compositor Core->>Display Broker: PresentSurfaceResp (result, direct_scanout_reason, release_fence_handle)
    Display Broker-->>Client: Success with release fence
```

---

## 4. Direct-Scanout Decision Flow

Direct-scanout selection is deterministic and avoids any hash-iteration or pointer ordering.

```mermaid
graph TD
    Start[Evaluate Direct Scanout] --> C1{Exactly 1 visible fullscreen surface?}
    C1 -- No --> F1[Composition Fallback: MULTIPLE_VISIBLE_SURFACES / NOT_FULLSCREEN]
    C1 -- Yes --> C2{Surface dimensions match display mode?}
    C2 -- No --> F2[Composition Fallback: SCALING_UNSUPPORTED]
    C2 -- Yes --> C3{Buffer has SCANOUT usage flag?}
    C3 -- No --> F3[Composition Fallback: USAGE_MISSING]
    C3 -- Yes --> C4{Does display have compatible direct-scanout plane?}
    C4 -- No --> F4[Composition Fallback: NO_COMPATIBLE_PLANE]
    C4 -- Yes --> Success[Select Direct Scanout: ELIGIBLE]
```

---

## 5. Capability and Ownership Model

- **Opaque 64-bit Handles**: Every display, lease, surface, buffer, and fence is referenced by a 64-bit handle:
  - Bits 0..15: Encoded Slot (Index + 1)
  - Bits 16..47: Generation (Monotonically incremented, prevents ABA stale lookup)
  - Bits 48..55: Resource Kind (Enforces type safety)
  - Bits 56..63: Handle ABI Version (Enforces layout compatibility)
- **Deny-by-Default Authorization**: All services require authorization via lease capabilities. Leases are coupled to the client endpoint PID. Unprivileged clients cannot mutate or present onto resources belonging to other leases.

---

## 6. Profile Enablement Matrix

The UI subsystem scales across all target device profiles of Bharat-OS:

| Feature | DESKTOP / MOBILE | AUTOMOTIVE | EDGE / APPLIANCE / KIOSK | TINY UI | RTOS / SAFETY |
| :--- | :---: | :---: | :---: | :---: | :---: |
| Full Composition | Yes | Yes | Optional | No (Disabled) | Prohibited |
| Direct-Scanout | Yes | Yes (Trusted Overlay) | Yes (Single Surface) | No (Disabled) | Prohibited |
| Surface state-machine | Yes | Yes | Limited | Restricted | Prohibited |
| Output Code | Enabled | Enabled | Enabled | Text only | Prohibited |

---

## 7. Resource Limits

- **Maximum Displays**: 4
- **Maximum Leases**: 8
- **Maximum Surfaces**: 16
- **Maximum Buffers**: 32
- **Maximum Fences**: 64
- **Maximum Planes per Display**: 4

All resources are tracked in bounded static arrays to prevent dynamic allocation overhead, memory fragmentation, or unbounded waits.

---

## 8. Security Considerations & Current Limitations

### Security Considerations
1. **Zero-Address Wire Contract**: No virtual or physical buffer addresses are ever exposed to the client in v2.
2. **Deny-by-Default**: Invalid or stale generation handles are immediately rejected.
3. **Protected Content Isolation**: Hardware overlays and protected memory domains are isolated from general CPU-read/write mappings.

### Current Limitations
1. **Software Fences**: The baseline reference implementation uses software-based fences.
2. **Single Display Active Scanout**: Headless simulation handles active scanout sequentially.
