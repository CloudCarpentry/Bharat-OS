
add_library(k_ds OBJECT
    bh_id_allocator.c
    bh_rcu_stub.c
    bh_range_tree.c
    bh_ring.c
    bh_seqlock.c
)
target_link_libraries(k_ds PRIVATE bharat_kernel_includes bharat_kernel_buildopts)
target_compile_options(k_ds PRIVATE $<$<COMPILE_LANGUAGE:C>:-ffreestanding> $<$<COMPILE_LANGUAGE:C>:-fno-builtin> $<$<COMPILE_LANGUAGE:C>:-fno-pic>)
