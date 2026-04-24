set(BHARAT_DRIVER_NET_SOURCES
    core/net_driver_core.c
)

if(BHARAT_ENABLE_DRIVER_VIRTIO_NET)
    list(APPEND BHARAT_DRIVER_NET_SOURCES virtio_net/virtio_net.c)
endif()

if(NOT BHARAT_DRIVER_NET_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_net.c "")
    list(APPEND BHARAT_DRIVER_NET_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_net.c)
endif()

add_library(bharatos_driver_net STATIC ${BHARAT_DRIVER_NET_SOURCES})
target_include_directories(bharatos_driver_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_net PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(bharatos_driver_net PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket bharat_generated_includes)
target_compile_options(bharatos_driver_net PRIVATE -ffreestanding -nostdlib -Wall)
