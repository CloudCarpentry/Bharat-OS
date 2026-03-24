set(BHARAT_DRIVER_I2C_HID_SOURCES)

if(BHARAT_ENABLE_DRIVER_I2C_HID)
    list(APPEND BHARAT_DRIVER_I2C_HID_SOURCES i2c_hid.c)
endif()

if(NOT BHARAT_DRIVER_I2C_HID_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_i2c_hid.c "")
    list(APPEND BHARAT_DRIVER_I2C_HID_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_i2c_hid.c)
endif()

add_library(bharatos_driver_input_i2c_hid STATIC ${BHARAT_DRIVER_I2C_HID_SOURCES})
target_include_directories(bharatos_driver_input_i2c_hid PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(bharatos_driver_input_i2c_hid PRIVATE -ffreestanding -nostdlib -Wall)