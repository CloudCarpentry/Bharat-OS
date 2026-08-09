
add_library(bharat_kernel_lib_ds STATIC
    base/string.c
    base/status.c
    ds/cuckoo_hash.c
    ds/radix_tree.c
    ds/urpc_ring.c
    sync/mcs_lock.c
)
target_link_libraries(bharat_kernel_lib_ds PRIVATE
    bharat_kernel_includes
    bharat_kernel_buildopts
)
