add_executable(servicemgr main.c servicemgr.c)
target_link_libraries(servicemgr PRIVATE bharat_user_buildopts bharat_freestanding bharat_libipc bharat_libstring)
target_include_directories(servicemgr PRIVATE ../../kernel/include ../../include)

bharat_configure_userspace_binary(servicemgr)