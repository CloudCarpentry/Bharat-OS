add_executable(process_manager main.c)
target_include_directories(process_manager PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)
target_compile_options(process_manager PRIVATE -ffreestanding -nostdlib -Wall)
target_link_libraries(process_manager PRIVATE urpc subsys_manager)
