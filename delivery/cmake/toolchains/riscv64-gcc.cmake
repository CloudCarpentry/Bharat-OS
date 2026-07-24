set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# GCC bare-metal RISC-V toolchain (Shakti/OpenSBI friendly)
set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)

find_program(BHARAT_RISCV_GCC_LD NAMES riscv64-unknown-elf-ld)
if(BHARAT_RISCV_GCC_LD)
    set(CMAKE_LINKER "${BHARAT_RISCV_GCC_LD}")
endif()

find_program(BHARAT_RISCV_GCC_OBJCOPY NAMES riscv64-unknown-elf-objcopy)
if(BHARAT_RISCV_GCC_OBJCOPY)
    set(CMAKE_OBJCOPY "${BHARAT_RISCV_GCC_OBJCOPY}")
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie")
set(CMAKE_ASM_FLAGS_INIT "-ffreestanding -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib")
