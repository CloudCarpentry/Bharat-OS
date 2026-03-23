add_library(k_sched OBJECT
    ../../src/sched.c
    ../../src/sched_deg.c
    ../../src/algo_matrix.c
    ../../src/ai_sched.c
    ../../src/lib/rbtree.c
)

target_include_directories(k_sched PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_sched PRIVATE bharat_freestanding bharat_kernel_buildopts)
