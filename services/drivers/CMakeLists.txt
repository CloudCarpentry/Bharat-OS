add_executable(drivers main.c)
target_include_directories(drivers PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)
target_compile_options(drivers PRIVATE -ffreestanding -nostdlib -Wall)
target_link_libraries(drivers PRIVATE urpc subsys_manager)
