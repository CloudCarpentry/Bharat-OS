cmake_minimum_required(VERSION 3.20)

add_executable(test_driver_registry
    test_driver_registry.c
    ../../../core/drivers/core/device_registry.c
    ../../../core/drivers/core/driver_registry.c
    ../../../core/drivers/core/event.c
    ../../../core/drivers/core/match.c
)

target_include_directories(test_driver_registry PRIVATE ../../../core/drivers/include)
target_include_directories(test_driver_registry PRIVATE ../../../interface/include)
target_include_directories(test_driver_registry PRIVATE ../../../core/drivers/core)
target_compile_definitions(test_driver_registry PRIVATE BHARAT_HOST_TEST)

add_executable(test_driver_match
    test_driver_match.c
    ../../../core/drivers/core/device_registry.c
    ../../../core/drivers/core/driver_registry.c
    ../../../core/drivers/core/event.c
    ../../../core/drivers/core/match.c
)

target_include_directories(test_driver_match PRIVATE ../../../core/drivers/include)
target_include_directories(test_driver_match PRIVATE ../../../interface/include)
target_include_directories(test_driver_match PRIVATE ../../../core/drivers/core)
target_compile_definitions(test_driver_match PRIVATE BHARAT_HOST_TEST)

add_executable(test_driver_event
    test_driver_event.c
    ../../../core/drivers/core/device_registry.c
    ../../../core/drivers/core/driver_registry.c
    ../../../core/drivers/core/event.c
    ../../../core/drivers/core/match.c
)

target_include_directories(test_driver_event PRIVATE ../../../core/drivers/include)
target_include_directories(test_driver_event PRIVATE ../../../interface/include)
target_include_directories(test_driver_event PRIVATE ../../../core/drivers/core)
target_compile_definitions(test_driver_event PRIVATE BHARAT_HOST_TEST)

add_test(NAME driver_registry_harden_test COMMAND test_driver_registry)
add_test(NAME driver_match_scoring_test COMMAND test_driver_match)
add_test(NAME driver_event_accounting_test COMMAND test_driver_event)
