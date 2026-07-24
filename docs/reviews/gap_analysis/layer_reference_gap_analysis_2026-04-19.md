---
title: Layer Reference Gap Analysis — Kernel/HAL/Arch/Platform/Services/Stacks/User-SDK/UIPC
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
  - gap_analysis
see_also:
  - README.md
---
# Layer Reference Gap Analysis — Kernel/HAL/Arch/Platform/Services/Stacks/User-SDK/UIPC

_Date:_ 2026-04-19  
_Scope:_ Static include/reference review across `.c/.h/.cc/.cpp/.hpp/.S` files.

## 1) Philosophy Baseline Used

This review applies Bharat-OS's stated direction: **small mechanism kernel, strict layering, policy in services**, and **UAPI as stable boundary**.

### Expected high-level dependency shape

- `arch` / `hal` / `platform` are hardware-mechanism layers.
- `kernel` may depend downward on hardware/mechanism layers, but not upward into user policy layers.
- `services`, `stacks`, `user`, `sdk` should prefer `interface/uapi/include/lib` contracts and avoid direct hardware headers.
- `lib` must remain reusable and avoid direct `hal` dependencies.

## 2) Full-Tree Scan Result

(Produced by `python3 tools/lint/check_layer_references.py`.)

- Total code/header files scanned: **1302**
- Files with layer-reference violations: **27**

## 3) Gap Summary (by layer edge)

| Source Layer | Target Layer | Count | Why this is a gap |
|---|---:|---:|---|
| `kernel` | `tests` | 15 | Production kernel references test headers directly; should be build-gated or isolated.
| `services` | `drivers` | 6 | Service contracts coupled to driver internals, weakening policy/mechanism split.
| `lib` | `hal` | 2 | Shared/public library headers depend on hardware abstraction directly.
| `stacks` | `hal` | 1 | Protocol stack bypasses service/UAPI boundary to hardware layer.
| `user` | `hal` | 1 | User app path directly includes HAL; should route through UAPI/SDK.
| `services` | `boot` | 1 | Service code consumes boot internals directly.
| `services` | `hal` | 1 | Service code takes direct HAL dependency rather than core/kernel/UAPI mediation.

## 4) Representative Wrong References (headers included)

### A. `kernel -> tests`
- `core/kernel/src/kernel_boot.c:34` includes `quality/tests/ktest.h`
- `core/kernel/src/boot/boot_selftest.c:3` includes `quality/tests/ktest.h`
- `core/kernel/src/quality/tests/ktest_capability.c:20` includes `quality/tests/ktest.h`

### B. `services -> drivers`
- `core/services/include/core/services/can/can_service_protocol.h:6` includes `core/drivers/can/can_controller.h`
- `core/services/include/core/services/actuator_mgr/actuator_mgr_protocol.h:2` includes `core/drivers/actuator/actuator_device.h`
- `core/services/include/core/services/sensor_hub/sensor_hub_protocol.h:2` includes `core/drivers/sensor/sensor_sample.h`

### C. `lib -> hal`
- `lib/include/game_engine_gfx.h:5` includes `corecore/hal/gpu.h`
- `lib/include/gui.h:5` includes `corecore/hal/gpu.h`

### D. `core/services/core/stacks/user -> corecore/hal/boot`
- `core/stacks/network/skb.c:2` includes `corecore/halcore/hal.h`
- `user/ui/fbui/core/fb_demo_app.c:5` includes `corecore/halcore/hal.h`
- `core/services/core/subsysmgr/subsys_test_runner.c:2` includes `boot/boot_args.h`
- `core/services/core/subsysmgr/subsys_test_runner.c:3` includes `corecore/halcore/hal.h`

## 5) Priority Fix Plan (cleanliness-first)

1. **P0 — isolate test headers from kernel production paths**
   - Move `quality/tests/ktest.h` to a kernel-private test interface (`core/kernel/include/quality/tests/`) or gate with dedicated test build flags.
2. **P0 — service protocol decoupling from drivers**
   - Replace direct driver type includes in `core/services/include/...` with neutral DTO types in `include/bharat/interface/uapi/...`.
3. **P1 — remove HAL includes from `lib/`, `user/`, `core/stacks/`**
   - Introduce adapter contracts in `uapi`/SDK and inject hardware execution via service or syscall boundary.
4. **P1 — remove service direct `boot`/`hal` coupling**
   - Define a boot-context UAPI payload handed from core/kernel/init contract instead of including `boot/*` headers.
5. **P2 — enforce in CI**
   - Run `tools/lint/check_layer_references.py --strict` in CI once above items are resolved.

## 6) Tooling Added in this change

A dedicated linter was added:

- `tools/lint/check_layer_references.py`

It scans all code/header files, classifies source/target layers from include paths, and supports deterministic architecture hygiene checks.
