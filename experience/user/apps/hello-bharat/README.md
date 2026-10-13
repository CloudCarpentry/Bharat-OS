# Hello Bharat-OS Sample

This folder contains a minimal "Hello World" application and a documented build template intended for developers starting with the Bharat-OS Native Application SDK.

## Reusable Template
The `CMakeLists.txt` file inside this folder demonstrates the standard mechanism for configuring a Bharat-OS user-space binary. It relies on the `bharat_configure_userspace_binary` function which handles the injection of the startup routine (`crt0`), basic memory layouts, and PIE (Position Independent Executable) settings.

You can use it as a starting point for developing other native applications.

## Cross-compilation Workflow for x86_64

To compile and execute this sample application as part of the Bharat-OS build workflow for x86_64, use the `nirmaan` build orchestrator from the repository root:

1. **Install Prerequisites**: Ensure you have `pyyaml` and `jsonschema` installed:
   ```bash
   pip install pyyaml jsonschema
   ```

2. **Build the Target**: Run `nirmaan` for the `x86_64_desktop_headless` target:
   ```bash
   ./nirmaan build delivery/targets/qemu/x86_64_desktop_headless.yaml --mode release
   ```

3. **Run the Output Artifact in QEMU**:
   Once successfully built, use `nirmaan run` (if supported) or explicitly invoke QEMU using the emitted artifacts in `build/x86_64-qemu-desktop-release/` to view the runtime output. (Note: If the launcher does not package/launch the user application automatically, you may need to build a packaged disk image manually or use a diagnostic boot parameter.)

## Build-time Checks
The template incorporates checks to verify that the core `bharat_configure_userspace_binary` function and dependencies like `bharat_libruntime`, `bharat_crt0`, and `bharat_syscall` are available in your CMake context, which indicates whether you're successfully integrated with the Bharat-OS build environment.
