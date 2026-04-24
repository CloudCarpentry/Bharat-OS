add_library(k_cap OBJECT
    ../../src/capability.c
    cap_policy.c
)

target_include_directories(k_cap PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_cap PRIVATE bharat_freestanding bharat_kernel_buildopts)
