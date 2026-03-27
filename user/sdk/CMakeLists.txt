cmake_minimum_required(VERSION 3.20)
project(BharatSDK C CXX ASM)

set(CMAKE_C_STANDARD 23) # Update to C23 as per guidelines

# Ensure we have our build options and freestanding libraries available,
# even if we are building the SDK out-of-tree, by linking them correctly or simulating them.
# The SDK depends on kernel headers.
include_directories(include)

# Allow standalone vs integrated compilation
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Standalone mode: we are building from user/sdk/ directly
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
else()
    # Integrated mode: we are building from the top-level repo
    include_directories(${CMAKE_SOURCE_DIR}/include)
endif()

# Allow building with multikernel features / Shakti
if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
    # Provide dummy bharat_exit since it's missing in RISC-V implementation for testing
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_exit.c "void bharat_exit(int status) { while(1); }")
    add_library(bharat STATIC lib/src/syscalls.c lib/src/riscv_syscalls.S ${CMAKE_CURRENT_BINARY_DIR}/dummy_exit.c)
else()
    # Provide a dummy syscall stub if compiling for testing natively (x86_64, arm64) since they lack ASM implementations
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_syscall.c "
        long bharat_syscall(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) { return -1; }
        void bharat_exit(int status) { while(1); }
    ")
    add_library(bharat STATIC lib/src/syscalls.c ${CMAKE_CURRENT_BINARY_DIR}/dummy_syscall.c)
endif()

# Provide headers publicly
target_include_directories(bharat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_include_directories(bharat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
else()
    target_include_directories(bharat PRIVATE ${CMAKE_SOURCE_DIR}/include)
endif()

# Note: bharat_user_buildopts and bharat_freestanding are defined in the main project.
# If building standalone, these might not exist. We link them optionally or create dummies.
if (TARGET bharat_user_buildopts AND TARGET bharat_freestanding)
    target_link_libraries(bharat PRIVATE bharat_user_buildopts bharat_freestanding)
else()
    # Add minimal freestanding flags if standalone
    target_compile_options(bharat PRIVATE -ffreestanding -nostdlib)
endif()

# Sample Application
add_executable(sample_app sample_app/main.c)
target_link_libraries(sample_app PRIVATE bharat bharat_user_buildopts bharat_freestanding)

# If building standalone, we need to ensure the executable is built without host stdlib
if (NOT TARGET bharat_user_buildopts)
    target_compile_options(sample_app PRIVATE -ffreestanding -nostdlib -fPIC)
    target_link_options(sample_app PRIVATE -nostdlib -pie)
else()
    target_compile_options(sample_app PRIVATE -fPIC)
    target_link_options(sample_app PRIVATE -pie)
endif()
