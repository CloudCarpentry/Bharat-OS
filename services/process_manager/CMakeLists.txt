add_executable(process_manager main.c process_manager.c)
target_link_libraries(process_manager PRIVATE bharat_user_buildopts bharat_freestanding bharat_libipc)
target_include_directories(process_manager PRIVATE ../../kernel/include ../../services/core/subsysmgr/include ../../lib/bharat_libtransport ../../include)

target_link_libraries(process_manager PRIVATE bharat_libtransport subsys_manager)

bharat_configure_userspace_binary(process_manager)
