# Bharat-OS SDK Structure

This repository contains the SDK surface for Bharat-OS, establishing the contract for platformization across Native, Linux, and Android personalities.

## Layout

* `core/`: Core SDK components for all app developers (manifest schema, generic IPC clients).
* `native/`: The true OS definition SDK (UI/window APIs, service registration, full capability-aware APIs).
* `compat/`: SDKs for bridging legacy code.
  * `linux/`: Syscall mapping docs, compatibility headers, unsupported API matrices.
  * `android/`: Stubbed Android HAL headers, ART bridging config.
* `runtime-host/`: SDK for runtime maintainers (embedding contract for Java, Python, Node, etc.).
* `bindings/`: Language specific SDK wrappers for interacting with the `native` SDK or `runtime-host`.
* `tools/`: Build and packaging tools (`bharat-pkg`, `bharat-run`, build targets).
