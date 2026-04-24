cmake_minimum_required(VERSION 3.20)

add_library(k_monitor OBJECT
    mon_vm_service.c
    mon_vm_dispatch.c
    mon_vm_state.c
)

target_include_directories(k_monitor PRIVATE
    ../../include
    ../../include/generated
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${BHARAT_BOOT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
)

target_link_libraries(k_monitor PRIVATE bharat_freestanding bharat_kernel_buildopts)
