add_library(driver_memblk STATIC
    memblk.c
)
target_include_directories(driver_memblk PRIVATE ../../include)
target_include_directories(driver_memblk PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
target_include_directories(driver_memblk PRIVATE ${CMAKE_SOURCE_DIR}/core/kernel/include)
target_link_libraries(driver_memblk PRIVATE
    bharat_user_buildopts
    bharat_freestanding
    bharat_generated_includes
    bharat_stacks_storage
)
