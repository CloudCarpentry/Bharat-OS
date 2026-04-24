add_library(k_trap OBJECT
    ../../src/trap/trap.c
    ../../src/trap/syscall.c
    ../../src/trap/trap_fault.c
    ../../src/fault_diag.c
)

target_include_directories(k_trap PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_trap PRIVATE bharat_freestanding bharat_kernel_buildopts)
