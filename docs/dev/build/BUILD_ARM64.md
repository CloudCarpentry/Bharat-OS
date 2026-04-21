# ARM64 Build Quickstart

## Build only

```bash
./build.sh build --target arm64_desktop_headless
```

```powershell
.\build.ps1 build --target arm64_desktop_headless
```

## Build + run

```bash
./build.sh all --target arm64_desktop_headless
```

```powershell
.\build.ps1 all --target arm64_desktop_headless
```

## YAML path form

```bash
./build.sh all --target-yaml tools/targets/qemu/arm64_desktop_headless.yaml
```

## Preset-first manual flow

```bash
cmake --preset arm64-edge
cmake --build --preset arm64-edge
```

For full CLI semantics, legacy flag compatibility (`--all`, `--build`, `--run`), packaging, and tests, see `HOST_BUILD_TEST_RUN_GUIDE.md`.
