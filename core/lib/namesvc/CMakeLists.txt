set(NAMESVC_CLIENT_SOURCES
    src/namesvc_client.c
)

add_library(bharat_libnamesvc STATIC ${NAMESVC_CLIENT_SOURCES})

target_include_directories(bharat_libnamesvc
    PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/lib/runtime/include
)

target_link_libraries(bharat_libnamesvc
    PUBLIC
    ipc
    bharat_freestanding
)
