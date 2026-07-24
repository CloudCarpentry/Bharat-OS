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
    include_directories(${CMAKE_SOURCE_DIR}/interface/include)
endif()

# Allow building with multikernel features / Shakti
# Provide dummy exit for testing
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_exit.c "void bharat_exit(int status) { while(1); }")
add_library(bharat STATIC lib/src/syscalls.c ${CMAKE_CURRENT_BINARY_DIR}/dummy_exit.c)

# Ensure the syscall backend is available
target_link_libraries(bharat PUBLIC bharat_syscall)
# Provide headers publicly
target_include_directories(bharat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    target_include_directories(bharat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
else()
    target_include_directories(bharat PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
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

# Note: In an integrated build, we should just use the global function
if (TARGET bharat_crt0)
    bharat_configure_userspace_binary(sample_app)
    target_link_libraries(sample_app PRIVATE bharat bharat_user_buildopts bharat_freestanding)
else()
    target_link_libraries(sample_app PRIVATE bharat)
    target_compile_options(sample_app PRIVATE -ffreestanding -nostdlib -fPIC)
    target_link_options(sample_app PRIVATE -nostdlib -pie)
endif()
