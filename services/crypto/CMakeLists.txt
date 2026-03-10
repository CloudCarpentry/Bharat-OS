add_executable(crypto main.c)
target_include_directories(crypto PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)
target_compile_options(crypto PRIVATE -ffreestanding -nostdlib -Wall)
target_link_libraries(crypto PRIVATE urpc subsys_manager)
