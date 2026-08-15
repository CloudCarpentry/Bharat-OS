---
title: "DEMO-P0-001 \u2014 Bharat GUI Shell User Guide & Architecture"
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# DEMO-P0-001 — Bharat GUI Shell User Guide & Architecture

## Overview

The **Bharat GUI Shell** (`experience/shell`) is the primary unprivileged graphical desktop experience for Bharat-OS. It provides a modern windowing and application launcher shell built on top of LVGL and the Bharat-OS Display Broker (`display_broker`).

---

## Target Workflow

```text
Boot Engine
    │
    ▼
Bharat-OS Splash Screen
    │
    ▼
Desktop / Launcher
 ├── System Info Screen
 ├── Device Viewer Screen
 ├── Processes View
 ├── Network Status
 ├── Hardware & Sensors
 └── Demo Apps
```

---

## 🚀 How to Run the GUI Shell Demo

### 1. Daily Commands

#### WSL / Linux / macOS
```bash
# Run x86_64 Graphical Desktop Shell (Interactive GTK Window)
./tools/build.sh run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml --interactive

# Run ARM64 Graphical Desktop Shell
./tools/build.sh run --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
```

#### Windows PowerShell
```powershell
.\tools\build.ps1 run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml --interactive
```

#### Nirmaan CLI
```bash
./nirmaan run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
```

---

## 🎨 Implemented Screens & Features

### 1. Splash Screen
- Branded **BHARAT-OS** banner with saffron accents.
- Platform readiness status (`Capability microkernel / Ready`).

### 2. Desktop / Launcher Screen
Interactive grid buttons to launch system applications:
- **System**: System metrics & hardware capabilities.
- **Devices**: Discovered system hardware & drivers.
- **Processes**: Task manager view.
- **Network**: Network interface monitor.
- **Hardware & Sensors**: Physical telemetry.
- **Demo Apps**: Unprivileged user application suite.

### 3. System Screen
Displays live platform information:
- **System Properties**: Architecture (`x86_64`/`arm64`), CPU Cores, Memory Total, Execution Profile, Runtime Mode, Kernel Build type.
- **Hardware Capabilities**:
  - `✓ MMU`
  - `✓ SMP`
  - `✓ Timer`
  - `✓ Atomic`
  - `✓ Display`
  - `✓ Network`
- **Live Telemetry Bar**: Real-time counter for **Uptime** (`HH:MM:SS`) and **Heap Usage** (`KB used / KB free`).

### 4. Device Viewer Screen
Lists recognized hardware drivers & operational status:
| Device Category | Driver | Status |
| :--- | :--- | :--- |
| `Display` | `virtio-gpu` | `READY` |
| `Keyboard` | `virtio-input` | `READY` |
| `Network` | `virtio-net` | `READY` |
| `Storage` | `virtio-blk` | `READY` |
| `Sensor` | `virtual-imu` | `READY` |
| `Accelerator` | `virt-npu` | `READY` |

---

## 📁 Code Ownership & Architecture

- **`experience/shell/`**:
  - `bharat_shell.h`: Shell data models, screen enum (`BH_SHELL_SCREEN_*`), navigation API.
  - `shell_ui.c`: LVGL screen layouts, styles, live timers, back-button handling, input focus groups.
  - `shell_model.c`: Hardware capability matrix and device table provider.
- **`experience/apps/gui_showcase/`**:
  - `main.c`: Display broker lease initialization, pointer/keyboard device creation, splash-to-launcher bootstrap loop.
- **`core/stacks/ui/adapters/lvgl/`**:
  - Native LVGL display & input driver adapters mapping to `display_broker` framebuffer leases.
