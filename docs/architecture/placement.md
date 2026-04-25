# Architecture Placement Contract

Bharat-OS enforces strict architectural boundaries to keep the system modular, portable, and secure. Contributors must adhere to these placement rules.

## 1. Directory Roles

* **`core/kernel/` = Mechanism Only**
  * Provides core primitives: memory mapping, CPU scheduling, IPC routing, and capability validation.
  * *Must not* contain policy (e.g., how to configure a network interface, how to structure a filesystem).
  * *Must not* contain board-specific logic or emulators (e.g., no `qemu` or `renode` logic here).

* **`core/services/` = Policy and Orchestration**
  * User-space processes running above the kernel.
  * Manages system state, enforces high-level policies, and orchestrates components (e.g., `netmgr`, `vm_manager`, `namesvc`).
  * *Must not* contain real hardware driver logic. Hardware control goes in `core/drivers/`.

* **`core/drivers/` = Hardware Control**
  * Hardware-specific implementations interacting with devices (e.g., UART, NICs, disk controllers).
  * This code talks to hardware registers or buses.

* **`core/platform/` = Board/Machine Composition**
  * Wires together components for specific hardware boards.
  * Defines device tree bindings or ACPI data specific to a machine.

* **`tools/` = Build, Run, and Deploy Logic**
  * Python scripts, run wrappers, and manifest builders.
  * Build logic must be decoupled from the runtime OS.

## 2. Examples of Violations to Avoid

* ❌ `core/kernel/src/qemu_hacks.c` (Emulator/board hacks in kernel)
* ❌ `core/services/netmgr/intel_e1000.c` (Hardware driver inside a service)
* ❌ `core/kernel/src/fs_policy.c` (Policy logic inside kernel)

## 3. Placement Lint

The codebase includes an automated lint script to catch obvious boundary violations. Run it manually or as part of CI:

```bash
python3 tools/lint/check_placement.py
```
