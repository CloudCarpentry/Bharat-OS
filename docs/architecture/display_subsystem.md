# Display and Output Architecture

The graphical and output presentation system in Bharat-OS defines multiple levels of abstraction. Unlike conventional OSes that treat the desktop compositor as the sole interface, we provide an intentionally layered approach that scales down to headless devices and up to complex multi-monitor desktop workstations.

## Design Philosophy

* **Do not make GUI mean only "desktop compositor".**
* Treat framebuffer graphics as a first-class citizen, as it is the most reliable, durable, and critical subsystem for bring-up, small devices, and edge deployments.
* Rely on modular subsystems so small devices (like IoT nodes or robots) do not drag the entirety of a graphics stack with them.

## Output Layer Tiers

### 1. Tier 0: Headless Mode
Applicable to devices with absolutely no graphical or text output mechanisms (e.g., storage nodes, headless network routers).
* Remote Shell and Management APIs.
* Telemetry and Remote Logging.
* Boot diagnostics sent purely over the network.

### 2. Tier 1: Text Console + Simple Display
The minimum useful visual stack for diagnostics and setup.
* VGA text console (x86_64).
* Serial console abstraction for all architectures.
* Simple font rasterizer over a linear framebuffer for kernel panics and logs.

### 3. Tier 2: Framebuffer Graphics Subsystem
This is the core target for many edge and embedded devices.
* **Framebuffer Core:** Handles pixel formats, strides, and memory mapping. Manages device registration and basic capabilities (e.g., rotation, backlight).
* **2D Primitive Layer:** Software-rendered lines, rectangles, blitting, and glyph drawing.
* **Basic Event Loop:** Tracks "dirty" screen rectangles for optimal flushing to hardware without requiring a full compositor.

### 4. Tier 3: Embedded Lightweight UI
For kiosks, industrial panels, and appliances, the UI lives just above the framebuffer.
* **FBUI Toolkit**: A lightweight toolkit of widgets currently implemented including labels, buttons, and progress bars.
* **FBUI Render Context**: Implements software-rendered fills, blitting, and line drawing.
* **FBUI Events**: Implements touch/key event routing, hit testing, and scene/view management.
* No complex window management, just full-screen or simple layout partitions.

### 5. Tier 4: Desktop Graphics Subsystem
Advanced environments requiring a compositor.
* Hardware-accelerated GPU modesetting and buffer sharing.
* Rich desktop shells and multi-window managers.
* Advanced input abstractions (e.g., Wayland-like protocols).

## Portability & Driver Interfaces

While Bharat-OS provides native capability-based APIs internally, the driver-facing abstractions are influenced by established models:

* **Linux DRM/KMS Concepts:** Framebuffer metadata, modesetting capabilities, and driver registration draw from `drm` but simplified for microkernel use.
* **Linux Input Subsystem:** Keycodes, absolute/relative axes, and touch tracking logic.
* **BSD Device Attachment Patterns:** Our driver initialization logic makes porting existing codebases simpler by mimicking standard probe/attach semantics.

## Accelerator Hooks

"Accelerators" are not strictly GPUs in the Bharat-OS model. Small devices often feature blitters, DMAs, DSPs, and NPUs. The `bharat_accel_ops` interface permits capabilities-based querying of these blocks. If a hardware blitter exists, the 2D primitive layer can transparently map it instead of using CPU-bound loops.

## Console vs UI and Layering Rules

It is critical to distinguish between the core trusted system console and higher-level user interfaces (or system UI services). The physical directory structure strictly enforces these layers.

### 1. Display Layering

*   **Hardware Display Drivers (`drivers/display/`):** Code that directly pokes registers or manages hardware components (modesetting, buffer allocation for actual screens, e.g., DRM or VirtIO GPU).
*   **Console Subsystem (`kernel/src/console/`):** Contains the stateful core console routing and rendering logic. This includes tracking character grids, cursors, wrapping, panning, and processing ANSI/newline characters. The `framebuffer_console` implementation natively draws text to a linear framebuffer using simple helper routines.
*   **System UI / Services (`services/system/...`):** Higher-level graphical shells, status bars, compositor proxies, or full-blown desktop managers.

### 2. Early/Trusted Console vs Service UI

The framebuffer console (`fb_console`) is a minimal system console path suitable for boot logs, early panics, and simple text output. It is **not** a general-purpose GUI or service. It operates inside the kernel/trusted core to ensure reliability during critical failures (like `panic()` paths). Service-level UIs, on the other hand, are run as independent processes and communicate via IPC.

### 3. Folder Ownership Matrix

To prevent subsystem entanglement, adhere to the following directory responsibilities:

*   `drivers/display/` &rarr; Real display hardware drivers.
*   `drivers/serial/` &rarr; UART/serial hardware drivers.
*   `kernel/src/console/` &rarr; Console routing, policy, panic paths, serial console backends, and framebuffer console backends.
*   `lib/ui/` &rarr; Reusable rendering primitives only (e.g., generic glyph or box drawing routines without state).
*   `services/system/...` &rarr; Higher-level system services, including user-mode UI daemons.

*(Note: In earlier repository structures, a `drivers/src/console/` folder existed as a transitional staging area. This pattern is invalid under the current architecture and has been fully refactored into the respective core subsystems.)*