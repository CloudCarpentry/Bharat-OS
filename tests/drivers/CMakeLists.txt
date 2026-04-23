cmake_minimum_required(VERSION 3.20)

add_executable(test_driver_baseline
    test_driver_baseline.c
    ../../drivers/core/device_registry.c
    ../../drivers/core/driver_registry.c
    ../../drivers/core/event.c
    ../../drivers/core/match.c
    ../../drivers/core/power.c
    ../../drivers/bus/gpio/gpio_core.c
    ../../services/core/devmgr/devmgr_skeleton.c
)

target_include_directories(test_driver_baseline PRIVATE ../../drivers/include)
add_test(NAME driver_baseline_test COMMAND test_driver_baseline)
