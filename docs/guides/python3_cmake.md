---
title: Using Python3_EXECUTABLE in CMake
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# Using Python3_EXECUTABLE in CMake

Bharat‑OS relies on Python 3 scripts for various build‑time checks (e.g. generating ABI offsets, testing trap‑frame ABI). To keep the build portable across Windows and Linux we use the `Python3_EXECUTABLE` variable provided by CMake's `FindPython3` module.

## Finding the interpreter

```cmake
# Locate a Python 3 interpreter (required for build scripts)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
```

`FindPython3` sets the cache variable `Python3_EXECUTABLE` to the absolute path of the interpreter found on the host system. It also defines the imported target `Python3::Interpreter` which can be used in `add_custom_command` or `add_custom_target`.

## Using it in custom commands

Example from the kernel `CMakeLists.txt`:

```cmake
add_custom_command(
    OUTPUT "${ASM_OFFSETS_H}"
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/abi/asm_offsets.py "${ASM_OFFSETS_DIR}/gen_asm_offsets.s" "${ASM_OFFSETS_H}"
    DEPENDS "${ASM_OFFSETS_DIR}/gen_asm_offsets.s" "${CMAKE_SOURCE_DIR}/tools/abi/asm_offsets.py"
    COMMENT "Generating ${ASM_OFFSETS_H}"
)

add_custom_target(check_trap_frame_abi
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/abi/test_trap_frame_abi.py
            "${ASM_OFFSETS_H}"
            ${BHARAT_ARCH_ROOT}/x86/x86_64/trap_entry.S
            ${BHARAT_ARCH_ROOT}/arm/arm64/trap_entry.S
            ${BHARAT_ARCH_ROOT}/arm/arm32/trap_entry.S
            ${BHARAT_ARCH_ROOT}/riscv/riscv64/trap_entry.S
            ${BHARAT_ARCH_ROOT}/riscv/riscv32/trap_entry.S
    DEPENDS generate_asm_offsets
    COMMENT "Checking five‑architecture trap frame ABI"
)
```

Because `Python3_EXECUTABLE` resolves to the correct interpreter on both Windows (e.g. `python.exe` from a virtual environment) and Linux/macOS, the same CMake code works without modification.

## Tips
- Ensure Python 3 is installed and available on the `PATH` before invoking the build.
- On Windows you may need to install the **Windows Python 3 launcher** (`py.exe`) or have `python.exe` in the environment.
- If you need a specific interpreter, you can override it on the command line:
  ```bash
  cmake -DPython3_EXECUTABLE=/usr/local/bin/python3 ..
  ```

This guide is added to `docs/guides/python3_cmake.md` and referenced from the repository's build documentation.
