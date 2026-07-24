---
title: x86_64 Build Quickstart
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
  - build
see_also:
  - README.md
---
# x86_64 Build Quickstart

## Build only

```bash
./build.sh build --target x86_64_desktop_headless
```

```powershell
.\build.ps1 build --target x86_64_desktop_headless
```

## Build + run

```bash
./build.sh all --target x86_64_desktop_headless
```

```powershell
.\build.ps1 all --target x86_64_desktop_headless
```

## YAML path form

```bash
./build.sh all --target-yaml tools/targets/qemu/x86_64_desktop_headless.yaml
```

## Preset-first manual flow

```bash
cmake --preset x86_64-dev
cmake --build --preset x86_64-dev
```

For full CLI semantics, legacy flag compatibility (`--all`, `--build`, `--run`), packaging, and tests, see `HOST_BUILD_TEST_RUN_GUIDE.md`.
