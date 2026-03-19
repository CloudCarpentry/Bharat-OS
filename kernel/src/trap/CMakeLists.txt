add_library(k_trap OBJECT
    ../../src/trap/trap.c
    ../../src/fault_diag.c
)

target_include_directories(k_trap PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_trap PRIVATE bharat_freestanding bharat_kernel_buildopts)
