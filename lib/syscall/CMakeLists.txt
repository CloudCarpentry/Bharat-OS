add_library(bharat_syscall STATIC syscall_stubs.c)
target_include_directories(bharat_syscall PUBLIC ${CMAKE_SOURCE_DIR}/include)
