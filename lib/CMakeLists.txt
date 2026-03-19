cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib C ASM)

# Add include directories for libc/posix
include_directories(posix/include)

# The actual C library implementation (e.g., musl) would be integrated here.
# For now, we create a dummy target to satisfy the build system.
add_library(c_posix_stub STATIC posix/stub.c)
target_compile_options(c_posix_stub PRIVATE -ffreestanding -nostdlib -Wall)

# URPC Library
add_library(urpc STATIC urpc/urpc.c)
target_include_directories(urpc PUBLIC urpc)
target_compile_options(urpc PRIVATE -ffreestanding -nostdlib -Wall)
add_subdirectory(elf)
add_subdirectory(syscall)

# Phase 1: Foundational Native Libraries
add_subdirectory(cap)
add_subdirectory(ipc)
add_subdirectory(runtime)
if(BHARAT_BUILD_NETWORK_STUBS)
    add_subdirectory(net)
    add_subdirectory(packet)
endif()

# Phase 2: Message/Wire Runtime
add_library(msg STATIC
    msg/wire.c
    msg/validate.c
    msg/crc.c
)
target_include_directories(msg PUBLIC ../subsys/include)
target_sources(msg PRIVATE msg/encode.c msg/decode.c)

# Phase 3: Capability on Wire
add_library(capwire STATIC
    capwire/cap_codec.c
    capwire/cap_validate.c
)
target_include_directories(capwire PUBLIC ../subsys/include)
target_link_libraries(capwire PRIVATE msg)

# Phase 4: Out Of Line (OOL) Descriptors
add_library(ool_desc STATIC
    ool/ool_codec.c
    ool/ool_validate.c
)
target_include_directories(ool_desc PUBLIC ../subsys/include)
target_link_libraries(ool_desc PRIVATE msg)

# Phase 5: Transport Abstraction
add_library(transport STATIC
    transport/loopback.c
)
target_include_directories(transport PUBLIC ../subsys/include)
target_link_libraries(transport PRIVATE msg)
