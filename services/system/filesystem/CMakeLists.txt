add_executable(system_filesystem main.c vfs_stub.c mount_mgr.c namespace_mgr.c fd_mgr.c)
target_link_libraries(system_filesystem PRIVATE bharat_user_buildopts bharat_freestanding bharat_libtransport subsys_manager bharat_stacks_storage)
target_include_directories(system_filesystem PRIVATE ../../../kernel/include ../../../services/core/subsysmgr/include ../../../lib/bharat_libtransport ../../../stacks/include)

bharat_configure_userspace_binary(system_filesystem)
