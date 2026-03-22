add_library(board_fabric STATIC board_fabric.c)
target_include_directories(board_fabric PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/drivers/include)
target_link_libraries(board_fabric PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_platform_headers)
