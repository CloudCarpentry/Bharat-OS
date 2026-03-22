add_library(bharatos_stack_network STATIC skb.c)
target_include_directories(bharatos_stack_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/lib/packet/include)
target_link_libraries(bharatos_stack_network PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket)
