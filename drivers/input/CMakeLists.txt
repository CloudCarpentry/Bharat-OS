add_subdirectory(i2c_hid)
add_subdirectory(virtio_input)

set(BHARAT_DRIVER_INPUT_SOURCES)
if(BHARAT_ENABLE_INPUT_GPIO_KEYS)
    list(APPEND BHARAT_DRIVER_INPUT_SOURCES gpio_keys.c)
endif()

if(NOT BHARAT_DRIVER_INPUT_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c "")
    list(APPEND BHARAT_DRIVER_INPUT_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
endif()

add_library(bharatos_driver_input_static STATIC ${BHARAT_DRIVER_INPUT_SOURCES})
target_include_directories(bharatos_driver_input_static PUBLIC ../include)
target_include_directories(bharatos_driver_input_static PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bharatos_driver_input_static PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_generated_includes)
target_compile_options(bharatos_driver_input_static PRIVATE -ffreestanding -nostdlib -Wall)

add_library(bharatos_driver_input INTERFACE)
target_link_libraries(bharatos_driver_input INTERFACE
  bharatos_driver_input_i2c_hid
  bharatos_driver_input_virtio_input
  bharatos_driver_input_static
)
target_include_directories(bharatos_driver_input INTERFACE i2c_hid virtio_input)