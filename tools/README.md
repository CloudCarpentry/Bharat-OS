# Bharat-OS `tools/` Guide

This directory contains the implementation for the Bharat-OS build delivery pipeline.

## What lives here

- `build.py` - authoritative CLI for build/package/run/flash/debug.
- `build.sh` / `build.ps1` - compatibility wrappers that forward to `build.py`.
- `build/` - Python modules for target resolve, validation, CMake execution, manifests.
- `package/` - packaging/transforms and manifest generation.
- `run/` - QEMU launcher.
- `flash/` - flashing backend integration (OpenOCD path).
- `targets/` - target loading/validation helpers (legacy compatibility layer).
- `debug.sh` / `debug.ps1` - helper debugger attach scripts.

Preferred target data locations during migration:

- `delivery/targets/qemu/*.yaml`
- `delivery/targets/target_matrix.json`

---

## Canonical usage

Use root wrappers from repository root:

```bash
./build.sh <subcommand> --target <name>
./build.sh <subcommand> --target-yaml <path>
```

```powershell
.\build.ps1 <subcommand> --target <name>
.\build.ps1 <subcommand> --target-yaml <path>
```

Subcommands: `configure`, `build`, `package`, `run`, `flash`, `debug`, `all`.

---

## Common workflows

### Build and run in one command

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

```bash
./build.sh all --target x86_64_desktop_headless
```

### Package only

```powershell
.\build.ps1 package --target x86_64_desktop_headless
```

```bash
./build.sh package --target x86_64_desktop_headless
```

### Run only (after prior build/package)

```powershell
.\build.ps1 run --target x86_64_desktop_headless
```

```bash
./build.sh run --target x86_64_desktop_headless
```

### Flash (board target)

```powershell
.\build.ps1 flash --target <board_target> --dry-run
```

```bash
./build.sh flash --target <board_target> --dry-run
```

### Legacy positional compatibility example

```powershell
.\build.ps1 x86_64_desktop_headless --run
```

Equivalent modern command:

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

---

## Where to read full docs

- Full build/install/preset guide: `../BUILD.md`
- High-level project quick start: `../README.md`
