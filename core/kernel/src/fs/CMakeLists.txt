add_library(k_fs OBJECT
    ../../src/fs/vfs.c
    ../../src/fs/mount.c
    ../../src/fs/file.c
    ../../src/fs/namespace.c
    ../../src/fs/path.c
    ../../src/fs/block.c
    ../../src/fs/object_store.c
)

target_include_directories(k_fs PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_fs PRIVATE bharat_freestanding bharat_kernel_buildopts)
