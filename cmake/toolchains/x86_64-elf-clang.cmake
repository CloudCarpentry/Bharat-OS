set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)

# On Windows, clang with x86_64-unknown-none-elf may dispatch link through gcc.
# Use a linux-gnu triple to keep clang driving lld directly while still linking
# a freestanding ELF via explicit linker script/options from kernel CMake.
if(CMAKE_HOST_WIN32)
    set(CMAKE_C_COMPILER_TARGET x86_64-unknown-linux-gnu)
    set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-linux-gnu)
    set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-linux-gnu)
else()
    set(CMAKE_C_COMPILER_TARGET x86_64-unknown-none-elf)
    set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-none-elf)
    set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-none-elf)
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Required by CMake to skip compiler checks that require a full standard library
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
