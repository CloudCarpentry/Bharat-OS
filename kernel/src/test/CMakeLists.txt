add_library(k_test_support OBJECT
    ../../src/tests/ktest_runner.c
    ../../src/tests/ktest_pmm.c
    ../../src/tests/ktest_slab.c
    ../../src/tests/ktest_hal_pt.c
    ../../src/tests/ktest_unmapped_fault.c
)

target_include_directories(k_test_support PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_test_support PRIVATE bharat_freestanding bharat_kernel_buildopts)
