add_executable(vm_manager main.c)
    target_link_libraries(vm_manager PRIVATE bharat_user_buildopts bharat_freestanding)
target_include_directories(vm_manager PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)

target_link_libraries(vm_manager PRIVATE urpc subsys_manager)
