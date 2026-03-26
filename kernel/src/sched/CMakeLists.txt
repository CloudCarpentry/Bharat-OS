add_library(k_sched OBJECT
    sched.c
    sched_deg.c
    algo_matrix.c
    ai_sched.c
)

target_sources(k_sched PRIVATE sched_test_support.c)

target_include_directories(k_sched PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/boot/include)
target_link_libraries(k_sched PRIVATE bharat_freestanding bharat_kernel_buildopts)
