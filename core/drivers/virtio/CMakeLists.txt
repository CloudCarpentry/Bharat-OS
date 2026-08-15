add_library(bharatos_driver_virtio STATIC
    core/virtqueue.c
    mmio/virtio_mmio.c
)

target_include_directories(bharatos_driver_virtio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/mmio
)
target_link_libraries(bharatos_driver_virtio PRIVATE
    bharat_kernel_buildopts
    bharat_freestanding
    bharat_generated_includes
)
target_compile_options(bharatos_driver_virtio PRIVATE
    -ffreestanding
    -nostdlib
    -Wall
)
