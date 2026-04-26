# Display Lease and Framebuffer Contract

## Responsibility Boundary
The **Display Broker** is the authoritative service for display ownership in Bharat-OS. It manages access to physical display devices and grants temporary **Display Leases** to clients (e.g., `boot_displayd`, `console`, `compositor`).

## Runtime Flow
1. **Discovery:** The Display Broker discovers display devices via `devmgr`.
2. **Lease Request:** A client sends a `RequestLease` IPC to the Broker with desired rights.
3. **Grant:** If approved, the Broker returns a `lease_id` and a simulated shared-memory pointer to the framebuffer.
4. **Handoff:** When a higher-priority service (like `console`) requests a lease, the Broker may revoke the current lease (held by `boot_displayd`).

## Capability/Right Model
Rights are governed by `BHARAT_DISPLAY_RIGHT_*` bitmasks:
- `LEASE`: Right to request/hold a lease.
- `PRESENT`: Right to trigger a display refresh.
- `WRITE`: Right to write to the framebuffer memory.
- `MODESET`: Right to change display resolution/format.

## Current Transitional Limitations
**IMPORTANT:** The current implementation uses a **transitional simulated shared framebuffer**.
- Framebuffer pointers are passed directly between services.
- No VM-backed isolation or hardware-enforced protection is currently in place.
- Revocation is cooperative or logical only; the Broker cannot yet "unmap" the buffer from the client.

## Next Phase Work
1. **VM-Backed Buffers:** Replace simulated pointers with kernel-mediated buffer capabilities and `mmap` calls.
2. **Hardware Flush:** Connect the `Present` IPC to real hardware page flipping or dirty-rect flushes.
3. **Multi-Display:** Extend the Broker to handle multiple independent display outputs.
