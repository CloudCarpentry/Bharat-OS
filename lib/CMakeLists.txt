cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib C ASM)

# ---------------------------------------------------------
# bharat_libcmin
# ---------------------------------------------------------
add_library(bharat_libcmin STATIC posix/stub.c)
target_include_directories(bharat_libcmin PUBLIC posix/include)
target_link_libraries(bharat_libcmin PUBLIC bharat_freestanding bharat_kernel_buildopts)

# ---------------------------------------------------------
# bharat_libtransport (URPC / Loopsback)
# ---------------------------------------------------------
add_library(bharat_libtransport STATIC urpc/urpc.c transport/loopback.c)
target_include_directories(bharat_libtransport PUBLIC urpc ../subsys/include)
target_link_libraries(bharat_libtransport PUBLIC bharat_freestanding bharat_kernel_buildopts)

# ---------------------------------------------------------
# Subdirectories that define specific libs
# ---------------------------------------------------------
add_subdirectory(elf)
add_subdirectory(syscall)
add_subdirectory(cap)
add_subdirectory(ipc)
add_subdirectory(runtime)

if(BHARAT_BUILD_NETWORK_STUBS)
    add_subdirectory(net)
endif()
add_subdirectory(packet)

# ---------------------------------------------------------
# bharat_libmsg (Wire/Validation/Encoding)
# ---------------------------------------------------------
set(BHARAT_LIBMSG_SOURCES
    msg/wire.c
    msg/validate.c
    msg/crc.c
    msg/crc_generic.c
    msg/encode.c
    msg/decode.c
)

# Optional arch-specific CRC backend based on detected or target architecture
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    # Only enable ARM CRC32 backend if the compiler natively supports it
    # We compile the backend anyway and conditionally link it, or we could add feature test.
    # For now, we will add the source and gate the #include/instructions in the file.
    list(APPEND BHARAT_LIBMSG_SOURCES msg/crc_aarch64.c)
endif()

add_library(bharat_libmsg STATIC ${BHARAT_LIBMSG_SOURCES})
if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set_source_files_properties(msg/crc_aarch64.c PROPERTIES COMPILE_FLAGS "-march=armv8-a+crc")
endif()
target_include_directories(bharat_libmsg PUBLIC ../subsys/include)
target_link_libraries(bharat_libmsg PUBLIC bharat_freestanding bharat_kernel_buildopts)

# Alias old 'msg' for compatibility during migration
add_library(msg ALIAS bharat_libmsg)

# ---------------------------------------------------------
# bharat_libcap (CapWire)
# ---------------------------------------------------------
add_library(bharat_libcap STATIC
    capwire/cap_codec.c
    capwire/cap_validate.c
)
target_include_directories(bharat_libcap PUBLIC ../subsys/include)
target_link_libraries(bharat_libcap PUBLIC bharat_libmsg bharat_freestanding bharat_kernel_buildopts)

# Alias for compatibility
add_library(capwire ALIAS bharat_libcap)

# ---------------------------------------------------------
# bharat_libdiag (OOL / diagnostics/ etc)
# ---------------------------------------------------------
add_library(bharat_libdiag STATIC
    ool/ool_codec.c
    ool/ool_validate.c
)
target_include_directories(bharat_libdiag PUBLIC ../subsys/include)
target_link_libraries(bharat_libdiag PUBLIC bharat_libmsg bharat_freestanding bharat_kernel_buildopts)

# Alias for compatibility
add_library(ool_desc ALIAS bharat_libdiag)
add_library(transport ALIAS bharat_libtransport)

