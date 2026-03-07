set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# Use Clang and lld by default as for other architectures
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_LINKER ld.lld)

set(CMAKE_C_COMPILER_TARGET riscv64-unknown-none-elf)
set(CMAKE_ASM_COMPILER_TARGET riscv64-unknown-none-elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Shakti-specific architecture and ABI options
set(SHAKTI_ARCH_FLAGS "-march=rv64imafdc -mabi=lp64d -mcmodel=medany")

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie ${SHAKTI_ARCH_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "-ffreestanding -fno-pic -fno-pie ${SHAKTI_ARCH_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib")
