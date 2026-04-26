add_library(bharat_syscall STATIC common/syscall.c)
target_include_directories(bharat_syscall PUBLIC ${CMAKE_SOURCE_DIR}/interface)
target_link_libraries(bharat_syscall PUBLIC bharat_freestanding)
