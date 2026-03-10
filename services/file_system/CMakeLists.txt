add_executable(file_system main.c)
target_include_directories(file_system PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)
target_compile_options(file_system PRIVATE -ffreestanding -nostdlib -Wall)
target_link_libraries(file_system PRIVATE urpc subsys_manager)
