cmake_minimum_required(VERSION 3.20)
project(ConsumerApp C)

find_package(BharatLibC CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE
    BharatLibC::Core
    BharatLibC::BSys
    BharatLibC::Alloc
    BharatLibC::StdioMin
)
