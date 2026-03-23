add_library(k_test_support OBJECT
    ktest_runner.c
    ktest_pmm.c
    ktest_slab.c
    ktest_hal_pt.c
    ktest_unmapped_fault.c
)

target_include_directories(k_test_support PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_test_support PRIVATE bharat_freestanding bharat_kernel_buildopts)
