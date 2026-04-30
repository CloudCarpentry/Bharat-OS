set(SERVICE_RUNTIME_SOURCES
    src/service_runtime.c
)

add_library(bharat_service_runtime STATIC ${SERVICE_RUNTIME_SOURCES})

target_include_directories(bharat_service_runtime
    PUBLIC
    include
)

target_link_libraries(bharat_service_runtime
    PUBLIC
    bharat_ipc
    bharat_uapi
)
