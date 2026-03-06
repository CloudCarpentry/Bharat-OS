# GUI Strategy — Bharat-OS

**Status:** Planned (user-space service, deferred post v1 kernel)

---

## Guiding Principle

The Bharat-OS microkernel provides **no graphics primitives**. A display server runs entirely in user-space as an ordinary capability-holding task, with no special kernel privileges. This aligns with the broader policy-out-of-kernel philosophy.

---

## Target Deployment Split

| Profile                       | GUI                           | Rationale                                    |
| ----------------------------- | ----------------------------- | -------------------------------------------- |
| **Bharat-RT**                 | None                          | Hard real-time; GUI latency is unacceptable  |
| **Bharat-Cloud**              | Optional (headless preferred) | Server workloads; GUI only for admin console |
| **Bharat-Desktop** _(future)_ | Wayland compositor            | End-user workstations                        |

---


## Performance Model for Deferred User-Space GUI

To preserve microkernel isolation while delivering desktop/mobile-class graphics performance, Bharat-OS adopts a split execution model:

1. **Zero-copy framebuffer sharing**
   - GPU memory allocations remain kernel-tracked objects, but are exported to `fbsrv`/`bharat-wm` as revocable capabilities.
   - User-space maps the shared surfaces directly and performs all scene graph + draw logic out of kernel.
   - Buffer handoff between Wayland clients and compositor is done by shared memory handle passing (capability transfer), not memcpy.

2. **Kernel compositing fast-path**
   - Normal composition policy stays in user-space, but the kernel may expose a hardware-overlay submission fast-path for trivial layers (cursor plane, full-screen video plane, static background).
   - This avoids extra round-trips for simple flips while still enforcing capability checks on which task can target which plane.

3. **GPU-accelerated render backend**
   - `gpu.h` HAL is expected to support explicit modern APIs (Vulkan-class queue model) so the compositor can do blur, transparency, and HiDPI scaling in shaders.
   - Software fallback remains possible for bring-up, but not as the steady-state desktop/mobile path.

This keeps GUI policy out of Ring-0 while allowing near-native throughput on modern SoCs.

---

## Phase 1 — Framebuffer Output (Bring-up)

The first graphics milestone is a simple linear framebuffer:

- QEMU's `-vga virtio` or `-device virtio-gpu` expose a framebuffer through VirtIO.
- A user-space **framebuffer daemon** (`fbsrv`) memory-maps the VirtIO GPU BAR via the IOMMU-protected driver model.
- Output: Bharat-OS boot splash screen (static PNG rendered via `stb_image`).
- No windowing, no input handling. Proves the display pipeline works end-to-end.

---

## Phase 2 — Wayland Compositor (Bharat-Desktop)

On top of the framebuffer server, a Wayland compositor runs as an unprivileged task:

```
[App 1 (Wayland client)]──┐
[App 2 (Wayland client)]──┤──►[bharat-wm (Wayland compositor)]──►[fbsrv]──►[VirtIO GPU]
[App 3 (Wayland client)]──┘
```

- **bharat-wm** implements the `wl_compositor` and `xdg_shell` Wayland protocols.
- Input events (keyboard, pointer) are received from the Input server (`inputsrv`) via capability IPC.
- Compositor and clients communicate over Wayland shared-memory buffers (no kernel involvement after channel setup).

**Candidate compositor base:** [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots/) (MIT licensed) as a user-space library linked into `bharat-wm`.

---

## Phase 3 — Theme & Branding Application

Once basic windowing works, apply the Bharat-OS brand palette (see `assets/branding/brand-guide.md`):

- Window decorations in navy `#0A0F1E` with saffron `#FF9933` focus rings.
- Custom cursor set with Ashoka Chakra motif.
- Boot splash: animated Bharat-OS logo fading in over 1.5 s.
- Default font stack: Inter (UI), JetBrains Mono (terminal).

---

---

## Future Platforms (Post-Desktop Roadmap)

These are **not** in scope for any current milestone but are first-class future targets and should not be architecturally closed off.

### Mobile (Bharat-Mobile)

Touch-first UI for smartphones and tablets.

| Concern      | Approach                                                                        |
| ------------ | ------------------------------------------------------------------------------- |
| Display      | GPU-composited Wayland with HiDPI / fractional scaling                          |
| Input        | Multitouch event protocol via `inputsrv` capability                             |
| Shell        | Minimal Wayland compositor optimised for small screens                          |
| Power        | GUI compositor must cooperate with the RT power governor to sleep/wake displays |
| Connectivity | Bluetooth HID for peripherals                                                   |

> Design constraint: the compositor must fit in < 4 MB RAM to leave headroom for foreground apps on low-end devices.

---

### Edge / Embedded Devices (Bharat-Edge UI)

Minimal graphical output for industrial panels, kiosks, and IoT gateways.

| Concern   | Approach                                                                   |
| --------- | -------------------------------------------------------------------------- |
| Display   | Direct framebuffer write (no full Wayland stack required)                  |
| Input     | Resistive touchscreen via evdev abstraction in `inputsrv`                  |
| Shell     | Single-app locked-down kiosk mode (one Wayland surface, no window manager) |
| Boot      | Splash-to-app in < 2 s (requires fbsrv + app launcher pre-linked)          |
| Footprint | GUI stack optional; disabled entirely for headless edge nodes              |

> Bharat-Edge UI reuses the Phase 1 framebuffer daemon (`fbsrv`) path — no new kernel changes needed.

---

### Automotive HMI (Bharat-Auto)

Safety-critical Human-Machine Interface for in-vehicle infotainment (IVI) and instrument cluster.

| Concern       | Approach                                                                            |
| ------------- | ----------------------------------------------------------------------------------- |
| Safety        | GUI process runs in an isolated capability domain; crash = no kernel impact         |
| Rendering     | Hard real-time rendering budget (16.6 ms / frame at 60 Hz guaranteed)               |
| Dual display  | Instrument cluster (safety) + IVI touchscreen (comfort) on separate compositors     |
| Compliance    | Target AGL (Automotive Grade Linux) Wayland protocol extensions for IVI-Shell       |
| Certification | GUI subsystem isolated from TCB; only the microkernel TCB is in scope for ISO 26262 |

> **Critical:** Automotive display pipeline must never share a scheduling domain with the RT brake/steer control loop. The compositor is a non-RT user-space task.

---

## Non-Goals (Kernel)

- No kernel-mode graphics driver.
- No DRM/KMS in the kernel; everything goes through VirtIO or a user-space DRM proxy.
- No GPU compute (CUDA/OpenCL) integration until cluster fabric is brought up.
- No platform-specific GUI code in the microkernel itself — all GUI targets are user-space services.
