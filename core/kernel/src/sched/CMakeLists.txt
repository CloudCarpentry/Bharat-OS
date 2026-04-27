add_library(k_sched OBJECT
    sched.c
    sched_core.c
    sched_runqueue.c
    sched_thread.c
    sched_wait.c
    sched_rt.c
    sched_gp.c
    sched_migrate.c
    sched_pi.c
    sched_ai_ingest.c
    sched_ai_guard.c
    sched_router.c
    sched_deg.c
    algo_matrix.c
    ai_sched.c
    sched_invariants.c
    cpu_partition.c
    cpu_partition_validate.c
    execution_mode.c
    execution_mode.c
    execution_mode.c
)

target_sources(k_sched PRIVATE sched_test_support.c)

target_include_directories(k_sched PRIVATE ../../include ${BHARAT_GENERATED_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_sched PRIVATE bharat_freestanding bharat_kernel_buildopts)

add_test(
    NAME sched.k_sched.build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target k_sched
)
add_test(
    NAME sched.k_test_support.build
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target k_test_support
)
