cmake_minimum_required(VERSION 3.20)
project(netstack C)

file(GLOB NETSTACK_SRC "src/*.c")
add_executable(netstack ${NETSTACK_SRC})
target_compile_options(netstack PRIVATE -Wall -ffreestanding -nostdlib )
target_link_options(netstack PRIVATE -nostdlib -no-pie)
# Link against necessary system primitives/stubs here in the future
