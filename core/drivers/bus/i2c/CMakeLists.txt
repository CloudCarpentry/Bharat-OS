set(BHARAT_DRIVER_I2C_CORE_SOURCES)

if(BHARAT_ENABLE_DRIVER_I2C_CORE)
    list(APPEND BHARAT_DRIVER_I2C_CORE_SOURCES i2c_core.c i2c_registry.c)
endif()

if(NOT BHARAT_DRIVER_I2C_CORE_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_i2c.c "")
    list(APPEND BHARAT_DRIVER_I2C_CORE_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_i2c.c)
endif()

add_library(bharatos_driver_bus_i2c STATIC ${BHARAT_DRIVER_I2C_CORE_SOURCES})
target_include_directories(bharatos_driver_bus_i2c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bharatos_driver_bus_i2c PRIVATE -ffreestanding -nostdlib -Wall)