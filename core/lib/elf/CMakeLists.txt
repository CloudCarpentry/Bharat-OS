add_library(elf_parser STATIC elf_parser.c)
target_include_directories(elf_parser PUBLIC ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(elf_parser PUBLIC bharat_freestanding)
