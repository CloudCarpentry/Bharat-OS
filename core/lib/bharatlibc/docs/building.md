# Building BharatLibC

BharatLibC supports two distinct build models:

## 1. Standalone Build

The standalone build compiles BharatLibC using a host compiler (or cross-compilation toolchain) without assuming access to the rest of the Bharat-OS monorepo.

To configure and build on the host for testing:
```bash
cmake -S core/lib/bharatlibc -B build/bharatlibc-host \
      -DBHARATLIBC_BACKEND=HOST \
      -DBHARATLIBC_PROFILE=host_test \
      -DBHARATLIBC_BUILD_TESTS=ON

cmake --build build/bharatlibc-host
ctest --test-dir build/bharatlibc-host --output-on-failure
```

## 2. Monorepo Integrated Build

When built as part of the Bharat-OS build, the parent CMake project integrates BharatLibC conditionally using:
```cmake
option(BHARAT_ENABLE_BHARATLIBC_NEXT "Build the isolated next-generation BharatLibC project" OFF)
```

The monorepo bridge automatically maps standard Bharat-OS architecture and profile choices to BharatLibC configuration parameters.
