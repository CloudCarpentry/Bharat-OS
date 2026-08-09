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
    ktest_smp_runtime.c
)

target_link_libraries(k_test_support PRIVATE
    bharat_kernel_includes
    bharat_kernel_buildopts
)
