## PAST WORK INVENTORY
- third_party UI inventory: `third_party/lvgl/upstream`
- LVGL pinned version: `9.5.0`
- Existing screen paths: `core/apps/ui_demo/screens/`
- Existing SDL backend: `tools/ui_simulator/`
- Existing headless backend: `quality/tests/ui/test_headless_render.c`
- Existing framebuffer/FBUI path: `experience/user/ui/fbui/`
- Existing Display Broker path: `core/services/system/display_broker/`
- Existing compositor path: `core/stacks/media/compositor/`
- Existing surface/buffer path: `interface/include/bharat/uapi/gui/surface_v2.h` & `interface/include/bharat/uapi/display/display_v2.h`
- Existing input-manager path: `core/services/device/inputmgr/`
- Existing QEMU display paths: `core/platform/boards/qemu-virt-x86_64/machine_display.c` (and variants)

## MISSING LINK FOUND
- **Before architecture:** LVGL was unlinked to the native system. Applications used SDL or framebuffer UI, with no native display broker/input manager connectivity to LVGL.
- **After architecture:** LVGL uses `lvgl_display_adapter.c` and `lvgl_input_adapter.c` to draw directly to Display Broker V2 compositor buffers and consume input manager events natively on device.

## EARLY → USERSPACE DISPLAY OWNERSHIP
- **current behavior:** The kernel maps early framebuffer `1:1` during boot and leaves it accessible. Later, userspace services claim the `CAP_TYPE_MEMORY` capability to re-map.
- **handoff behavior:** Userspace must check for a valid display capability, map the display, and clear any legacy early FBUI state. There is a potential overlap if both draw concurrently.
- **remaining dependency:** An explicit kernel handoff primitive to cleanly silence early framebuffer writing or revoke early boot mapping access before the Wayland/display broker compositor fully takes over is missing.

## POST-MERGE INTEGRATION
- exact runtime-model/root wiring required: Wire `gui_showcase` as the primary startup graphical executable.
- The `gui_showcase` will need the actual BIDL client implementations mapped in place of the current weak IPC stub linkage inside the standalone native application once `P0-002` dynamic capabilities settle.
