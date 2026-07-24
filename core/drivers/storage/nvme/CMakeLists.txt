set(BHARAT_DRIVER_NVME_SOURCES)

if(BHARAT_ENABLE_DRIVER_NVME)
    list(APPEND BHARAT_DRIVER_NVME_SOURCES nvme_core.c nvme_admin.c nvme_queue.c)
endif()

if(NOT BHARAT_DRIVER_NVME_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_nvme.c "")
    list(APPEND BHARAT_DRIVER_NVME_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_nvme.c)
endif()

add_library(bharatos_driver_storage_nvme STATIC ${BHARAT_DRIVER_NVME_SOURCES})
target_include_directories(bharatos_driver_storage_nvme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bharatos_driver_storage_nvme PRIVATE -ffreestanding -nostdlib -Wall)