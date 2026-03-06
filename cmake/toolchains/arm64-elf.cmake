set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_LINKER ld.lld)

set(CMAKE_C_COMPILER_TARGET aarch64-unknown-none-elf)
set(CMAKE_ASM_COMPILER_TARGET aarch64-unknown-none-elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie")
set(CMAKE_ASM_FLAGS_INIT "-ffreestanding -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -nostdlib")
