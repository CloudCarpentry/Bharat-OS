set(BHARAT_DRIVER_VIRTIO_INPUT_SOURCES)
if(BHARAT_ENABLE_DRIVER_VIRTIO_INPUT)
    list(APPEND BHARAT_DRIVER_VIRTIO_INPUT_SOURCES virtio_input.c)
endif()

if(NOT BHARAT_DRIVER_VIRTIO_INPUT_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_virtio_input.c "")
    list(APPEND BHARAT_DRIVER_VIRTIO_INPUT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_virtio_input.c)
endif()

add_library(bharatos_driver_input_virtio_input STATIC ${BHARAT_DRIVER_VIRTIO_INPUT_SOURCES})
target_include_directories(bharatos_driver_input_virtio_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bharatos_driver_input_virtio_input PRIVATE ${CMAKE_SOURCE_DIR}/drivers/input/include)
target_compile_options(bharatos_driver_input_virtio_input PRIVATE -ffreestanding -nostdlib -Wall)
