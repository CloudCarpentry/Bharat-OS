set(BHARAT_DRIVER_REGISTRY_SOURCES
    driver_version_registry/driver_version_registry.c
)

if(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY OR BHARAT_ENABLE_DRIVER_BOARD_FABRIC)
    if(BHARAT_ENABLE_DRIVER_GENERIC_REGISTRY)
        list(APPEND BHARAT_DRIVER_REGISTRY_SOURCES generic_registry/generic_registry.c)
    endif()
endif()

add_library(bharatos_driver_registry STATIC ${BHARAT_DRIVER_REGISTRY_SOURCES})
target_include_directories(bharatos_driver_registry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_registry PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(bharatos_driver_registry PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket bharat_generated_includes)
target_compile_options(bharatos_driver_registry PRIVATE -ffreestanding -nostdlib -Wall)
