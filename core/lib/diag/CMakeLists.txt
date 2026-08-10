# SPDX-License-Identifier: MIT
option(BHARAT_DIAG_ENABLED "Build the bounded diagnostic event mechanism" ON)
set(BHARAT_DIAG_RING_CAPACITY 64 CACHE STRING "Default diagnostic ring slots")
set(BHARAT_DIAG_MAX_PAYLOAD 256 CACHE STRING "Maximum diagnostic event payload bytes")
option(BHARAT_DIAG_BOOT_EVIDENCE "Enable boot evidence consumers" ON)

add_library(bharat_diag STATIC src/diag_ring.c src/diag_emit.c)
target_include_directories(bharat_diag PUBLIC include ${CMAKE_SOURCE_DIR}/interface/uapi ${CMAKE_SOURCE_DIR}/interface/include)
target_compile_definitions(bharat_diag PUBLIC
    BHARAT_DIAG_ENABLED=$<BOOL:${BHARAT_DIAG_ENABLED}>
    BHARAT_DIAG_MAX_PAYLOAD=${BHARAT_DIAG_MAX_PAYLOAD}
    BHARAT_DIAG_RING_CAPACITY=${BHARAT_DIAG_RING_CAPACITY}
    BHARAT_DIAG_BOOT_EVIDENCE=$<BOOL:${BHARAT_DIAG_BOOT_EVIDENCE}>)
target_link_libraries(bharat_diag PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_diag)
