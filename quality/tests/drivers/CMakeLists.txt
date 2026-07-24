cmake_minimum_required(VERSION 3.20)

add_executable(test_driver_baseline
    test_driver_baseline.c
    ../../core/drivers/core/device_registry.c
    ../../core/drivers/core/driver_registry.c
    ../../core/drivers/core/event.c
    ../../core/drivers/core/match.c
    ../../core/drivers/core/power.c
    ../../core/drivers/bus/gpio/gpio_core.c
    ../../core/services/core/devmgr/devmgr_skeleton.c
)

target_include_directories(test_driver_baseline PRIVATE ../../core/drivers/include)
add_test(NAME driver_baseline_test COMMAND test_driver_baseline)
