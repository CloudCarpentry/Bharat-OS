# SDK & Toolchain Roadmap (Gap Analysis)

## Current Maturity Level
The current Bharat-OS SDK and toolchain have moved from primitive scripts to structured, error-checked execution. The `build.sh` and `build.ps1` scripts provide predictable behaviors, and sample apps are integrated into the SDK build.

However, further improvements are needed for production-grade robustness.

## Identified Gaps & Tasks

### 1. Developer Templates & App Generation
- **Problem:** Creating a new application requires manually setting up `CMakeLists.txt`, copying linker flags, and creating entry points.
- **Proposed Fix:** Introduce a CLI tool (e.g., `bosh new-app my_app`) that generates scaffolding for a new Bharat-OS application.
- **Priority:** High
- **Impact:** Massively reduces developer friction when starting new services.

### 2. ABI/API Stability and Versioning
- **Problem:** Syscall interfaces and SDK headers are currently unversioned. Changes to the kernel can break user-space compilation silently.
- **Proposed Fix:** Implement strict semantic versioning for `libbharat_sdk.a` and enforce ABI contract tests in CI.
- **Priority:** Medium
- **Impact:** Prevents kernel-user space regressions.

### 3. Debugger Integration
- **Problem:** Developers must manually configure GDB to connect to QEMU when debugging applications.
- **Proposed Fix:** Add a `--debug-qemu` flag to the SDK and Kernel build scripts that automatically launches QEMU with `-s -S` and spawns a connected GDB instance.
- **Priority:** High
- **Impact:** Essential for diagnosing user-space faults and kernel panics.

### 4. CI Coverage for User Apps
- **Problem:** Currently, CI focuses heavily on kernel building and unit tests, but doesn't necessarily compile and link complex user-space topologies.
- **Proposed Fix:** Add end-to-end SDK compilation and application execution to the standard GitHub Actions matrix.
- **Priority:** High
- **Impact:** Guarantees SDK usability is not broken by kernel-side header changes.

### 5. Unified Output and Artifact Packaging
- **Problem:** Artifacts are dumped into `build/<arch>`. Distributing the SDK to third parties is manual.
- **Proposed Fix:** Create a `make install` or `cmake --install` step that bundles headers and `libbharat_sdk.a` into a distributable tarball/zip format.
- **Priority:** Medium
- **Impact:** Enables third-party development without the full Bharat-OS source tree.
