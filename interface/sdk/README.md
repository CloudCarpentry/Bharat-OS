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

## Bharat SDK v0.1 quick start

The root of this directory is also a standalone CMake project. Its public
headers under `include/bharat/` do not include kernel-private headers.

```sh
interface/sdk/bin/bh-clang interface/sdk/examples/hello/main.c -o hello.bh
./hello.bh
```

For installed CMake consumption, configure and install the SDK, then use
`find_package(BharatSDK CONFIG REQUIRED)` and link `Bharat::sdk`:

```sh
cmake -S interface/sdk -B build/sdk -DBHARAT_SDK_BUILD_TESTS=ON
cmake --build build/sdk
ctest --test-dir build/sdk --output-on-failure
cmake --install build/sdk --prefix "$PWD/bharat-sdk"
```

SDK v0.1 provides working hosted implementations of console output, logging,
monotonic time, sleep, exit, and system information. Process/thread, IPC,
device, sensor, accelerator, and capability APIs are the initial narrow source
surface but return `BH_ERR_UNSUPPORTED` on the hosted backend. Examples report
that limitation rather than simulating native Bharat-OS behavior.

### Boundary and authority rules

The SDK owns no kernel object and maintains no mutable global registry. Handles
and capabilities are fixed-width opaque values. Resource acquisition carries
explicit authority, and unavailable operations fail closed. Future native
bindings must consume generated syscall numbers and stable UAPI or service
contracts; they must not include `core/kernel` headers or reproduce syscall
tables.
