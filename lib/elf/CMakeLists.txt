add_library(elf_parser STATIC elf_parser.c)
target_include_directories(elf_parser PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(elf_parser PUBLIC bharat_freestanding bharat_kernel_buildopts)
