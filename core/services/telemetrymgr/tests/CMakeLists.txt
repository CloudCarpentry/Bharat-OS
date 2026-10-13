cmake_minimum_required(VERSION 3.20)

add_executable(test_telemetrymgr
    test_telemetrymgr.c
    ../registry.c
    ../../common/thermal_policy.c
)

target_include_directories(test_telemetrymgr PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/services/include
    ..
)

target_link_libraries(test_telemetrymgr PRIVATE
    bharat_user_buildopts
    bharat_freestanding
    bharat_libcmin
    bharatc
    bharat_service_runtime
    bharat_libipc
    bharat_libruntime
)

target_compile_definitions(test_telemetrymgr PRIVATE TEST_ENV)

add_test(NAME test_telemetrymgr COMMAND test_telemetrymgr)
