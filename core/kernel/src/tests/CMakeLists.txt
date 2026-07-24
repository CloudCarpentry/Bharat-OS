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
    ktest_string.c
    ktest_memops.c
    ktest_rt_sched.c
    ktest_irq_domain.c
    ktest_sched_ipi.c
)

target_include_directories(k_test_support PRIVATE ../../include ${BHARAT_GENERATED_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_test_support PRIVATE bharat_freestanding bharat_kernel_buildopts)
