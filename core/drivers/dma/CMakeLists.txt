add_library(bharatos_driver_dma STATIC edu_dma.c)
target_include_directories(bharatos_driver_dma PUBLIC ../include)
target_include_directories(bharatos_driver_dma PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/core/boot/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
)
target_link_libraries(bharatos_driver_dma PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_generated_includes)
target_compile_options(bharatos_driver_dma PRIVATE -ffreestanding -nostdlib -Wall)
