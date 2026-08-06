# LVGL for Bharat-OS

This directory contains a vendored, immutable snapshot of LVGL (Light and Versatile Graphics Library).

*   **Upstream:** https://github.com/lvgl/lvgl
*   **License:** MIT

## Rules for this directory
1. Do not modify files in `upstream/`.
2. Any unavoidable patch must be documented in `PATCHES.md`.
3. Bharat-specific configurations (`lv_conf.h`), ports, and adapters live in `core/stacks/ui/adapters/lvgl/`, not here.
