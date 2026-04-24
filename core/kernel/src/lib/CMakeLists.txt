
add_library(bharat_kernel_lib_ds STATIC
    base/string.c
    base/status.c
    ds/cuckoo_hash.c
    ds/radix_tree.c
    ds/urpc_ring.c
    sync/mcs_lock.c
)
target_include_directories(bharat_kernel_lib_ds PRIVATE
    ${BHARAT_KERNEL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/interface/include
    ${BHARAT_LIB_ROOT}/include
)
target_link_libraries(bharat_kernel_lib_ds PRIVATE bharat_freestanding bharat_kernel_buildopts)
