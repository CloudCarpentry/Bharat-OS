set(BHARAT_DRIVER_BUS_SOURCES)

if(BHARAT_ENABLE_DRIVER_CLUSTER_BUS)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES cluster_bus/cluster_bus.c)
endif()

if(BHARAT_ENABLE_DRIVER_I2C_CORE)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES i2c/i2c_core.c i2c/i2c_registry.c)
endif()

if(BHARAT_ENABLE_DRIVER_SPI_CORE)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES spi/spi_core.c spi/spi_registry.c)
endif()

if(BHARAT_ENABLE_GPIO)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES gpio/gpio_core.c)
endif()

if(BHARAT_ENABLE_PINCTRL)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES gpio/pinctrl_core.c)
endif()

if(BHARAT_ENABLE_PWM)
    list(APPEND BHARAT_DRIVER_BUS_SOURCES gpio/pwm_core.c)
endif()

# We only create the target if there are sources, or we create an empty static lib or object lib.
# Actually let's just create an empty library if no sources are selected, to satisfy the link.
if(NOT BHARAT_DRIVER_BUS_SOURCES)
    # create a dummy file to satisfy add_library
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.c "")
    list(APPEND BHARAT_DRIVER_BUS_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy.c)
endif()

add_library(bharatos_driver_bus STATIC ${BHARAT_DRIVER_BUS_SOURCES})
target_include_directories(bharatos_driver_bus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_bus PRIVATE ${CMAKE_SOURCE_DIR}/lib/packet/include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/kernel/include)
target_link_libraries(bharatos_driver_bus PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket bharat_generated_includes)
target_compile_options(bharatos_driver_bus PRIVATE -ffreestanding -nostdlib -Wall)