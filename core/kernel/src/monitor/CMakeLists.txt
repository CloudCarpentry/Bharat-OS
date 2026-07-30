cmake_minimum_required(VERSION 3.20)

add_library(k_monitor OBJECT
    mon_vm_state.c
    ../../../services/system/monitord/mon_vm_service.c
    ../../../services/system/monitord/mon_vm_dispatch.c
)

target_include_directories(k_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../include
    ${BHARAT_GENERATED_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${BHARAT_BOOT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/services/core/subsysmgr/include
)

target_link_libraries(k_monitor PRIVATE bharat_freestanding bharat_kernel_buildopts)
