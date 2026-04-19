# SDK Tools

Build, packaging, and diagnostic tools for Bharat-OS development.

## Core Tooling Concepts

* `bharat`: The main CLI entry point for interacting with a connected Bharat-OS device or emulator.
* `bharat-pkg`: Tooling for packing applications based on their manifest. Translates manifest capabilities into deployable bundles.
* `bharat-run`: Emulation runner/launcher.
* `bharat-debug`: Diagnostic bridge for GDB/LLDB or language-specific profilers.

## Build Target Model
Defines cross-compilation toolchains, sysroots, and target triples per architecture and personality (e.g., `x86_64-bharat-native`, `aarch64-bharat-linuxcompat`).
