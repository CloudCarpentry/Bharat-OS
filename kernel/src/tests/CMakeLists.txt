add_library(k_test_support OBJECT
    ktest_runner.c
    ktest_pmm.c
    ktest_slab.c
    ktest_hal_pt.c
    ktest_unmapped_fault.c
    ktest_capability.c
    ktest_accel_mem.c
    ktest_virt_accel.c
    ktest_urpc_handoff.c
    security/test_crypto_registry.c
    ktest_string.c
    ktest_rt_sched.c
)

target_include_directories(k_test_support PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/boot/include)
target_link_libraries(k_test_support PRIVATE bharat_freestanding bharat_kernel_buildopts)
