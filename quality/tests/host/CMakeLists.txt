cmake_minimum_required(VERSION 3.20)

add_executable(test_servicemgr
    "${CMAKE_CURRENT_SOURCE_DIR}/test_servicemgr.c"
    "${CMAKE_SOURCE_DIR}/core/services/servicemgr/servicemgr.c"
    "${CMAKE_SOURCE_DIR}/core/lib/ipc/src/ipc.c"
)

target_include_directories(test_servicemgr PRIVATE
    "${CMAKE_SOURCE_DIR}/interface/include"
    "${CMAKE_SOURCE_DIR}/interface"
    "${CMAKE_BINARY_DIR}/generated/include"
    "${CMAKE_SOURCE_DIR}/core/lib/ipc/include"
    "${CMAKE_SOURCE_DIR}/core/lib/cap/include"
    "${CMAKE_SOURCE_DIR}/core/kernel/include"
    "${CMAKE_SOURCE_DIR}/core/lib/include"
    "${CMAKE_SOURCE_DIR}/core/lib/runtime/include"
    "${CMAKE_SOURCE_DIR}/core/lib/namesvc/include"
    "${CMAKE_SOURCE_DIR}/core/services/servicemgr"
)

target_compile_definitions(test_servicemgr PRIVATE BHARAT_HOST_TEST)

add_test(NAME host_test_servicemgr COMMAND test_servicemgr)
