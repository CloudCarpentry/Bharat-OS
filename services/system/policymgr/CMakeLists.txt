add_executable(policymgr main.c policymgr.c)
target_link_libraries(policymgr PRIVATE bharat_user_buildopts bharat_freestanding bharat_libipc)
target_include_directories(policymgr PRIVATE ../../../kernel/include ../../../services/core/subsysmgr/include ../../../lib/bharat_libtransport ../../../include)

target_link_libraries(policymgr PRIVATE bharat_libtransport subsys_manager)

bharat_configure_userspace_binary(policymgr)
