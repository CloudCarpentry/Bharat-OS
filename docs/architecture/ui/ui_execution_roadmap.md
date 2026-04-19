# Bharat-OS UI/Display Roadmap (Ticket-Oriented, Production-Grade)

**Status:** Proposed next-task execution plan  
**Scope:** UI/media architecture as first-class Bharat-OS area, explicitly out of kernel policy space.

## Current Execution Focus (Started)

- **Selected core task:** **P1 — Production-grade display/input class model**.
- **Why this task first:** it unblocks `devmgr`, `displayd`, `inputd`, and every higher-level UI stack by defining a stable service-visible hardware contract.
- **Implementation started in this change-set:**
  - canonical display/input subclasses added to shared device class UAPI
  - generic display/input capability-bit registry added
  - canonical display/input capability structs and input event wire shape added
  - host test added for UI class/capability contract stability

---

## 1) Layer Contract (Non-Negotiable)

- **Kernel/HAL/Drivers:** mechanism only (buffer primitives, sync/timing, DMA/IOMMU hooks, device interrupts, capability enforcement hooks).
- **Services:** orchestration and policy (`uimgr`, `displayd`, `inputd`, `compositord`, `boot_displayd`, `trusted_uid`).
- **Stacks:** reusable UI/media composition and adapters (`stacks/ui/*`, `stacks/media/*`, `stacks/vehicle/ui`).
- **Personalities:** compatibility and domain shell behavior (`compat/*/gui`, `domain/*/ui`).

> Rule: no widget policy, no shell policy, no window/compositor policy in `kernel/`.

---

## 2) Repository Target Shape

```text
drivers/
  display/
    fb/
    drm_like/
    gpu/
    panel/
    bridge/
    hdmi/
    lvds/
    mipi_dsi/
  input/
    touch/
    keypad/
    rotary/
    ir/
    can_input/

services/
  core/
    devmgr/
    uimgr/
  system/
    boot_displayd/
    displayd/
    inputd/
    compositord/
    trusted_uid/

stacks/
  ui/
    core/
    lcd/
    hmi/
    toolkit/
    adapters/
      lvgl/
      sdl/
      qt/
      wayland/
  media/
    compositor/
    video/
    audio_sync/
    overlay/
  vehicle/
    ui/

personalities/
  compat/
    linux/gui/
    android/gui/
  domain/
    automotive/ui/
    medical/ui/
  common/
    theme_runtime/
```

---

## 3) Profile and Runtime Selection Contract

### Compile-time profile gates

- `BHARAT_UI_NONE`
- `BHARAT_UI_CHARLCD`
- `BHARAT_UI_LVGL`
- `BHARAT_UI_HMI_LITE`
- `BHARAT_UI_COMPOSITOR`
- `BHARAT_UI_WAYLAND_ADAPTER`
- `BHARAT_UI_QT_ADAPTER`
- `BHARAT_UI_ANDROID_PERSONALITY`

### Hardware capability gates

- GPU present
- display controller count
- overlay plane support
- secure overlay support
- IOMMU/DMA path
- input modalities (touch/rotary/IR/CAN)

### Runtime mode selection (policy manager)

- normal shell
- safe-mode shell
- diagnostics/factory shell
- kiosk shell
- cluster-only shell
- recovery shell
- medical locked-down workflow shell

### Mandatory runtime fallback chain

- rich UI unavailable -> HMI shell
- HMI shell unavailable -> boot/tiny UI
- display unavailable -> serial/diag fallback

---

## 4) Capability Model Backlog (UI-Specific)

Define capability tokens and enforcement checks for:

- display enumerate
- mode set
- create surface
- submit buffer
- input read
- input inject
- privileged overlay
- screenshot capture
- rear-view/camera display
- trusted alert present
- haptic/audio alert trigger

---

## 5) Execution Order (Waves)

### Wave 1

- D1: GUI architecture document
- D2: GUI roadmap and profile matrix
- P1: Production-grade display/input class model

### Wave 2

- P2: Production-grade boot display + tiny UI path

### Wave 3

- P3: Production-grade surface/buffer contract + direct-scanout path

### Wave 4

- P4: Production-grade HMI shell baseline + trusted alert path

### Wave 5

- P5: LVGL adapter
- P6: Wayland/Linux GUI adapter

---

## 6) Production-Grade Task Specs

## P1 — Production-grade display/input class model

### Goal
Create the canonical device-class foundation for GUI-related hardware.

### Deliverables
- canonical display device class
- canonical input device class
- class/subclass/capability descriptors
- input event model
- display mode/plane/framebuffer capability structs

### Likely code areas
- `drivers/include/driver_core.h`
- `drivers/display/include/...`
- `drivers/input/include/...`
- `services/core/devmgr/` (or equivalent)
- `uapi/` or `idl/services/` for service-visible contracts
- `tests/`

### New classes/subclasses
- `DISPLAY_CLASS_PANEL`
- `DISPLAY_CLASS_FRAMEBUFFER`
- `DISPLAY_CLASS_GPU`
- `DISPLAY_CLASS_VIDEO_OUT`
- `INPUT_CLASS_TOUCH`
- `INPUT_CLASS_KEYPAD`
- `INPUT_CLASS_ROTARY`
- `INPUT_CLASS_IR`
- `INPUT_CLASS_CAN_CONTROL`

### Must expose
- resolution
- pixel formats
- refresh ranges
- plane count
- overlay support
- direct-scanout support
- secure overlay support
- input source type
- coordinate mode
- key/rotary semantics
- event timestamps

### Acceptance
- services query display/input capabilities generically
- no board-specific UI assumptions in higher layers
- host/unit tests cover capability descriptors
- no policy in kernel

## P2 — Production-grade boot display + tiny UI path

### Goal
Implement first shippable GUI path for low-end devices and safe-mode/boot flows.

### Deliverables
- `services/system/boot_displayd/`
- tiny renderer stack
- framebuffer/panel output contract
- text + icon + status page rendering
- simple event integration from input
- safe-mode/recovery screens
- watchdog/error display hooks

### Placement
- service: `services/system/boot_displayd/`
- stack/runtime: `stacks/ui/lcd/`
- drivers stay in `drivers/display/` and `drivers/input/`

### Features
- monochrome and RGB framebuffer support
- character LCD backend option
- page-based UI (non-windowed)
- static assets from build
- deterministic low-memory rendering
- no compositor dependency

### Acceptance
- boot logo/diagnostics/recovery pages render on supported backend
- keypad or touch navigates simple screens
- no GPU dependency
- survives low-memory mode
- safe-mode path works when rich UI stack is disabled

## P3 — Production-grade surface/buffer contract + direct-scanout path

### Goal
Implement mechanism layer required by compositor/media/rich shell work.

### Deliverables
- surface contract
- buffer lifecycle contract
- fence/sync hooks
- display plane assignment API
- direct scanout for full-screen bypass
- trusted overlay reservation hooks
- capability-checked buffer submit path

### Likely paths
- `stacks/ui/core/`
- `stacks/media/compositor/` or `stacks/media/core/`
- `services/system/displayd/`
- `uapi/ipc/`
- `idl/services/`
- kernel hooks only if strictly mechanism

### Must define
- create/destroy surface
- allocate/import/export buffer
- present/retire lifecycle
- fence wait/signal
- plane compatibility query
- overlay eligibility
- full-screen direct-scanout
- fallback-to-composition behavior

### Acceptance
- one surface renders fullscreen directly without compositor
- buffer ownership/lifetime explicit
- no policy leakage into kernel
- capability checks on surface create/display bind/present
- tests cover lifecycle and plane selection

## P4 — Production-grade HMI shell baseline + trusted alert path

### Goal
Provide first production shell for medical/automotive/appliance class systems.

### Deliverables
- `services/core/uimgr/`
- `services/system/inputd/`
- `services/system/displayd/`
- `services/system/trusted_uid/`
- `stacks/ui/hmi/`

### Functional scope
- session and seat management
- input routing
- screen/view model
- HMI shell runtime
- alarm/confirm overlay path
- audit hooks for privileged alerts
- safe-state/restricted-mode shell

### Acceptance
- trusted alert preempts normal screen
- input focus rules enforced
- privileged overlay capability-gated
- normal shell failure does not kill trusted alert path
- locked-down mode supported

## P5 — LVGL adapter (optional but strong)

### Goal
Best first open-source GUI integration for embedded/HMI profiles.

### New area
- `stacks/ui/adapters/lvgl/`

### Acceptance
- LVGL demo runs through Bharat-OS display/input contracts
- no direct board-specific driver reach-around

## P6 — Wayland/Linux GUI adapter (optional, after P3)

### Goal
Host richer Linux-like GUI stacks in optional profiles.

### Acceptance
- Linux-personality GUI hosting path exists
- display/compositor stack hosts surface clients cleanly
- feature remains optional by profile/build

---

## 7) Ticket-Ready Agent Tasks

1. **Agent Task 1** — Write Bharat-OS GUI architecture and layering contract.
   - DoD: paths named, boundaries explicit, open-source hosting approach and trusted path documented.
2. **Agent Task 2** — Write GUI roadmap with profile/capability matrix.
   - DoD: phase plan, compile-time/runtime model, dependencies and release gates.
3. **Agent Task 3** — Implement canonical display/input classes and descriptors.
   - DoD: class model merged, service query path exists, tests pass.
4. **Agent Task 4** — Implement `boot_displayd` and tiny LCD/framebuffer stack.
   - DoD: boot/safe-mode UI renders; low-memory path and basic input navigation work.
5. **Agent Task 5** — Implement surface/buffer lifecycle + direct-scanout path.
   - DoD: fullscreen direct present works; lifecycle explicit; tests cover ownership and plane selection.
6. **Agent Task 6** — Implement `uimgr`, `inputd`, `displayd`, `trusted_uid` baseline.
   - DoD: HMI shell works; trusted overlay preemption and capability gates enforced.
7. **Agent Task 7** — Integrate LVGL adapter.
   - DoD: LVGL demo renders through Bharat-OS stack without board coupling.

---

## 8) Strong Implementation Rules (Apply to Every Task)

- no widget/window/compositor policy in kernel
- no real drivers inside services
- no board-specific assumptions in stacks
- all UI-facing IPC must be capability-checked
- build/profile flags must permit full feature exclusion on tiny devices
- runtime fallback chain must be implemented
- each task must include tests and documentation updates

---

## 9) Definition of Done (Roadmap-Level)

1. UI directory scaffolding aligns with folder-boundary contracts.
2. Kernel diffs are mechanism-only and policy-free.
3. Compile-time flags and runtime selection logic are documented and wired.
4. Capability checks cover UI-critical service operations.
5. At least one tiny profile and one compositor profile build successfully.
6. Wave-ordered tasks land with tests and doc updates.

---

## 10) Explicit Non-Goals

- No widget toolkit policy in kernel.
- No shell/runtime theme policy in kernel.
- No window manager semantics in kernel.
- No forced replacement of existing GUI ecosystems.
- No monolithic single-shell model for automotive/medical critical systems.
