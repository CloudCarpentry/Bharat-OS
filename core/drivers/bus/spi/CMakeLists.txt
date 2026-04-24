set(BHARAT_DRIVER_SPI_CORE_SOURCES)

if(BHARAT_ENABLE_DRIVER_SPI_CORE)
    list(APPEND BHARAT_DRIVER_SPI_CORE_SOURCES spi_core.c spi_registry.c)
endif()

if(NOT BHARAT_DRIVER_SPI_CORE_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_spi.c "")
    list(APPEND BHARAT_DRIVER_SPI_CORE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_spi.c)
endif()

add_library(bharatos_driver_bus_spi STATIC ${BHARAT_DRIVER_SPI_CORE_SOURCES})
target_include_directories(bharatos_driver_bus_spi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bharatos_driver_bus_spi PRIVATE -ffreestanding -nostdlib -Wall)