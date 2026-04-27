cmake_minimum_required(VERSION 3.20)

option(BHARAT_BUILD_HEADLESS_COMPOSITOR "Build headless compositor test backend" ON)

if(BHARAT_BUILD_HEADLESS_COMPOSITOR)
    add_library(bharat_headless_compositor STATIC
        compositor/backends/headless/headless_backend.c
    )
    target_include_directories(bharat_headless_compositor PUBLIC ${CMAKE_SOURCE_DIR}/interface/include  ${CMAKE_SOURCE_DIR}/core/kernel/include
        ${CMAKE_CURRENT_SOURCE_DIR}/compositor/backends/headless
    )
    target_link_libraries(bharat_headless_compositor PUBLIC bharat_freestanding)
endif()
