add_executable(vm_manager main.c vm_manager.c)
target_link_libraries(vm_manager PRIVATE bharat_user_buildopts bharat_freestanding bharat_libipc)
target_include_directories(vm_manager PRIVATE ../../kernel/include ../../include)

bharat_configure_userspace_binary(vm_manager)