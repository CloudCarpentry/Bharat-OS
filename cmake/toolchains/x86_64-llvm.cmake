set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_LINKER ld.lld)

# On Windows, clang with x86_64-unknown-none-elf may dispatch link through gcc.
# Use a linux-gnu triple to keep clang driving lld directly while still linking
# a freestanding ELF via explicit linker script/options from kernel CMake.
if(WIN32)
	set(CMAKE_C_COMPILER_TARGET x86_64-unknown-linux-gnu)
	set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-linux-gnu)
else()
	set(CMAKE_C_COMPILER_TARGET x86_64-unknown-none-elf)
	set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-none-elf)
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_C_FLAGS_INIT "-ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie")
set(CMAKE_ASM_FLAGS_INIT "-ffreestanding -fno-pic -fno-pie")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -fuse-ld=lld -v")
