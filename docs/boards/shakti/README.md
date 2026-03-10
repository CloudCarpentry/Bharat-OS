# SHAKTI Board Support Notes

This folder tracks SHAKTI board-specific support status for Bharat-OS.

## Planned boards

- `artix7_35t`
- `artix7_100t`
- `nexys_video`

## Expected artifacts per board

- board manifest/descriptor;
- linker profile(s);
- OpenOCD config;
- GDB init script;
- upload/flash helper wiring;
- bring-up checklist results.

## Status convention

- `planned`
- `in-progress`
- `booting`
- `validated`

Keep board-specific constants isolated to platform/backend layers; do not place board maps into scheduler/mm/ipc/core capability code.
