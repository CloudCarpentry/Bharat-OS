# Extraction Guide

BharatLibC is engineered for future standalone repository extraction.

## Steps for Extraction

1. Copy the `core/lib/bharatlibc/` directory out of the monorepo.
2. In the parent monorepo, remove `add_subdirectory(core/lib/bharatlibc)`.
3. In the parent monorepo, replace internal linkages to the target subdirectory with:
   ```cmake
   find_package(BharatLibC CONFIG REQUIRED)
   ```
4. The standalone project remains instantly buildable and publishable as it maintains its own isolated `CMakeLists.txt`, presets, and testing suite.
