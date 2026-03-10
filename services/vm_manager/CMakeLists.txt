add_executable(vm_manager main.c)
target_include_directories(vm_manager PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)
target_compile_options(vm_manager PRIVATE -ffreestanding -nostdlib -Wall)
target_link_libraries(vm_manager PRIVATE urpc subsys_manager)
