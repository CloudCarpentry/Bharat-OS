add_executable(file_system main.c vfs_stub.c)
target_link_libraries(file_system PRIVATE bharat_user_buildopts bharat_freestanding bharat_libtransport subsys_manager)
target_include_directories(file_system PRIVATE ../../kernel/include ../../subsys/include ../../lib/bharat_libtransport)
