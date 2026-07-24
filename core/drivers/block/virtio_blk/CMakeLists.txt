add_library(driver_virtio_blk STATIC
    virtio_blk.c
)
target_include_directories(driver_virtio_blk PRIVATE ../../include)
target_include_directories(driver_virtio_blk PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
target_include_directories(driver_virtio_blk PRIVATE ${CMAKE_SOURCE_DIR}/core/stacks/include)
target_link_libraries(driver_virtio_blk PRIVATE
    bharat_user_buildopts
    bharat_freestanding
    bharat_generated_includes
    bharat_stacks_storage
)
