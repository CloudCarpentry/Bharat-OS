add_library(k_time OBJECT
    ktime.c
)

target_include_directories(k_time PRIVATE
    ${BHARAT_KERNEL_INCLUDE_DIR}
    ${BHARAT_GENERATED_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${BHARAT_LIB_ROOT}/include
    ${BHARAT_BOOT_INCLUDE_DIR}
    ${BHARAT_HAL_ROOT}/include
)

target_link_libraries(k_time PRIVATE bharat_freestanding bharat_kernel_buildopts)
