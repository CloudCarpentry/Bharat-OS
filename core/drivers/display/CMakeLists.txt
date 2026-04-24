set(BHARAT_DRIVER_DISPLAY_SOURCES)

if(BHARAT_ENABLE_DRIVER_VIRTIO_GPU)
    list(APPEND BHARAT_DRIVER_DISPLAY_SOURCES virtio_gpu/virtio_gpu.c)
endif()

if(BHARAT_ENABLE_DRIVER_DRM_CORE)
    list(APPEND BHARAT_DRIVER_DISPLAY_SOURCES drm/drm_core.c drm/drm_registry.c)
endif()

if(NOT BHARAT_DRIVER_DISPLAY_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_display.c "")
    list(APPEND BHARAT_DRIVER_DISPLAY_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_display.c)
endif()

add_library(bharatos_driver_display STATIC ${BHARAT_DRIVER_DISPLAY_SOURCES})
target_include_directories(bharatos_driver_display PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_display PRIVATE ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(bharatos_driver_display PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_generated_includes)
target_compile_options(bharatos_driver_display PRIVATE -ffreestanding -nostdlib -Wall)
