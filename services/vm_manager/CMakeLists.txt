add_executable(vm_manager main.c)
    target_link_libraries(vm_manager PRIVATE bharat_user_buildopts bharat_freestanding)
target_include_directories(vm_manager PRIVATE ../../kernel/include ../../subsys/include ../../lib/bharat_libtransport)

target_link_libraries(vm_manager PRIVATE bharat_libtransport subsys_manager)

bharat_configure_userspace_binary(vm_manager)
