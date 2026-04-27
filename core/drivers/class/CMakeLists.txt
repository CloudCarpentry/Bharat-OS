set(BHARAT_DRIVER_CLASS_SOURCES)

if(BHARAT_ENABLE_DRIVER_CAN_CORE)
    list(APPEND BHARAT_DRIVER_CLASS_SOURCES
        can/can_controller_core.c
        can/can_filter_compile.c
    )
endif()

if(NOT BHARAT_DRIVER_CLASS_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_class.c "")
    list(APPEND BHARAT_DRIVER_CLASS_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_class.c)
endif()

add_library(bharatos_driver_class STATIC ${BHARAT_DRIVER_CLASS_SOURCES})
target_include_directories(bharatos_driver_class PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_include_directories(bharatos_driver_class PRIVATE ${CMAKE_SOURCE_DIR}/core/stacks/include  ${CMAKE_SOURCE_DIR}/core/stacks/include ${CMAKE_SOURCE_DIR}/core/lib/packet/include ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/kernel/include)
target_link_libraries(bharatos_driver_class PRIVATE bharat_kernel_buildopts bharat_freestanding bharat_libpacket bharat_generated_includes)
target_compile_options(bharatos_driver_class PRIVATE -ffreestanding -nostdlib -Wall)

if (BHARAT_AUTOMOTIVE_PROFILE)
    # Generic Sensor Driver Core Library
    add_library(bharat_driver_sensor STATIC sensor/sensor_core.c)
    target_include_directories(bharat_driver_sensor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bharat_driver_sensor PRIVATE bharat_libruntime)

    # Generic Actuator Driver Core Library
    add_library(bharat_driver_actuator STATIC actuator/actuator_core.c)
    target_include_directories(bharat_driver_actuator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bharat_driver_actuator PRIVATE bharat_libruntime)

    # Generic Motor Driver Core Library
    add_library(bharat_driver_motor STATIC
        motor/motor_control_core.c
        motor/pwm_motor_backend.c
        motor/qei_backend.c
    )
    target_include_directories(bharat_driver_motor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    target_link_libraries(bharat_driver_motor PRIVATE bharat_libruntime)
endif()
