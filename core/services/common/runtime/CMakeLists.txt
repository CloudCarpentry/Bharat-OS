set(SERVICE_RUNTIME_SOURCES
    src/service_runtime.c
)

add_library(bharat_service_runtime STATIC ${SERVICE_RUNTIME_SOURCES})

target_include_directories(bharat_service_runtime
    PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/cap/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
)

target_link_libraries(bharat_service_runtime
    PUBLIC
    ipc
    bharat_freestanding
)
