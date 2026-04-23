set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_unknown.c)
if(ARCH STREQUAL "x86_64")
    set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_x86_64.c)
elseif(ARCH STREQUAL "arm64")
    set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_arm64.c)
elseif(ARCH STREQUAL "arm32")
    set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_arm32.c)
elseif(ARCH STREQUAL "riscv64")
    set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_riscv64.c)
elseif(ARCH STREQUAL "riscv32")
    set(SYSTEM_FILESYSTEM_ARCH_SRC fs_arch_riscv32.c)
endif()

add_executable(system_filesystem
    main.c
    ${SYSTEM_FILESYSTEM_ARCH_SRC}
    vfs_stub.c
    mount_mgr.c
    namespace_mgr.c
    fd_mgr.c
)
target_link_libraries(system_filesystem PRIVATE bharat_user_buildopts bharat_freestanding bharat_libtransport subsys_manager bharat_stacks_storage driver_virtio_blk bharat_libstring)
target_include_directories(system_filesystem PRIVATE ../../../kernel/include ../../../services/core/subsysmgr/include ../../../lib/bharat_libtransport ../../../stacks/include)

bharat_configure_userspace_binary(system_filesystem)
