# SHAKTI Workflow Wrappers

These scripts provide SHAKTI-style entry points while reusing Bharat-OS build and board metadata.

- `list-targets` — lists `shakti-*` entries from `tools/boards/boards.json`.
- `build` — wraps `tools/build.sh` for riscv64 SHAKTI board targets.
- `upload` — placeholder upload wrapper using existing flash helper fallback.
- `debug` — OpenOCD/GDB convenience wrapper (expects board cfg under `platform/shakti/boards/<target>/openocd.cfg`).

They are intentionally thin wrappers so that architecture and kernel internals remain platform-neutral.
