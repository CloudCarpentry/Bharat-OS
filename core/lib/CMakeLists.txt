cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib C ASM)

# ---------------------------------------------------------
# bharat_libcmin
# ---------------------------------------------------------
add_library(bharat_libcmin STATIC posix/stub.c)
target_include_directories(bharat_libcmin PUBLIC posix/include)
target_link_libraries(bharat_libcmin PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_libcmin)

# ---------------------------------------------------------
# bharat_libtransport (URPC / Loopsback)
# ---------------------------------------------------------
add_library(bharat_libtransport STATIC urpc/urpc.c transport/loopback.c)
target_include_directories(bharat_libtransport PUBLIC
    urpc
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
    runtime/include
)
target_link_libraries(bharat_libtransport PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_libtransport)

# ---------------------------------------------------------
# bharat_version_api
# ---------------------------------------------------------
add_library(bharat_version_api STATIC version_api.c)
target_include_directories(bharat_version_api PUBLIC ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(bharat_version_api PUBLIC bharat_freestanding bharat_generated_includes)
bharat_configure_userspace_library(bharat_version_api)

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
add_subdirectory(rbtree)
add_subdirectory(string)

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
target_include_directories(bharat_libmsg PUBLIC
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
    runtime/include
)
target_link_libraries(bharat_libmsg PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_libmsg)

# Alias old 'msg' for compatibility during migration
add_library(msg ALIAS bharat_libmsg)

# ---------------------------------------------------------
# bharat_libcap (CapWire)
# ---------------------------------------------------------
add_library(bharat_libcap STATIC
    capwire/cap_codec.c
    capwire/cap_validate.c
)
target_include_directories(bharat_libcap PUBLIC ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include)
target_link_libraries(bharat_libcap PUBLIC bharat_libmsg bharat_freestanding)
bharat_configure_userspace_library(bharat_libcap)

# Alias for compatibility
add_library(capwire ALIAS bharat_libcap)

# ---------------------------------------------------------
# bharat_libdiag (OOL / diagnostics/ etc)
# ---------------------------------------------------------
add_library(bharat_libdiag STATIC
    ool/ool_codec.c
    ool/ool_validate.c
)
target_include_directories(bharat_libdiag PUBLIC ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include)
target_link_libraries(bharat_libdiag PUBLIC bharat_libmsg bharat_freestanding)
bharat_configure_userspace_library(bharat_libdiag)

# Alias for compatibility
add_library(ool_desc ALIAS bharat_libdiag)
add_library(transport ALIAS bharat_libtransport)

# ---------------------------------------------------------
# Bharat SDK Components
# ---------------------------------------------------------
set(BH_SDK_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include
    ${CMAKE_SOURCE_DIR}/interface
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib
)

add_library(bharatsys STATIC
    syscall/common/syscall.c
)
target_include_directories(bharatsys PUBLIC ${BH_SDK_INCLUDES})
target_link_libraries(bharatsys PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharatsys)

add_library(bharatc STATIC
    runtime/src/crt0.c
    runtime/src/malloc/malloc.c
    runtime/src/string/string.c
    runtime/src/stdio/stdio.c
    runtime/src/errno/errno.c
    runtime/src/bharat/bh_native.c
)
target_include_directories(bharatc PUBLIC ${BH_SDK_INCLUDES})
target_link_libraries(bharatc PUBLIC bharatsys bharat_freestanding)
bharat_configure_userspace_library(bharatc)

# Installation targets for sysroot
install(TARGETS bharatc bharatsys DESTINATION lib)
install(DIRECTORY runtime/include/ DESTINATION include)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/interface/uapi DESTINATION include/interface)

# ---------------------------------------------------------
# Layer boundary enforcement
# ---------------------------------------------------------
# Enforce the architecture rule from docs/architecture/shared/lib_architecture.md:
# targets defined under top-level core/lib/ must not link against kernel-private
# targets (e.g., bharat_kernel_*).
function(_bharat_assert_no_kernel_links DIRPATH)
    get_property(_targets DIRECTORY "${DIRPATH}" PROPERTY BUILDSYSTEM_TARGETS)

    foreach(_target IN LISTS _targets)
        foreach(_prop IN ITEMS LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
            get_target_property(_links "${_target}" "${_prop}")
            if(NOT _links)
                continue()
            endif()

            foreach(_link IN LISTS _links)
                if(_link MATCHES "(^|;)bharat_kernel|(^|;)kernel")
                    message(FATAL_ERROR
                        "Library layering violation: target '${_target}' in core/lib/ links against '${_link}'. "
                        "Shared lib/ targets must not depend on kernel-private targets.")
                endif()
            endforeach()
        endforeach()
    endforeach()

    get_property(_subdirs DIRECTORY "${DIRPATH}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        _bharat_assert_no_kernel_links("${_subdir}")
    endforeach()
endfunction()

_bharat_assert_no_kernel_links("${CMAKE_CURRENT_SOURCE_DIR}")
