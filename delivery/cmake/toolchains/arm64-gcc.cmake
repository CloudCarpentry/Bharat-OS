set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm64)

set(CMAKE_C_COMPILER aarch64-none-elf-gcc)
set(CMAKE_ASM_COMPILER aarch64-none-elf-gcc)
set(CMAKE_LINKER aarch64-none-elf-ld)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie")
set(CMAKE_ASM_FLAGS_INIT "-ffreestanding -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib")
