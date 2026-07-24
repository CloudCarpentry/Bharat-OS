# Platform layer: SoC/board wiring and machine abstractions
add_library(bharat_platform_headers INTERFACE)
target_include_directories(bharat_platform_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(qemu)
add_subdirectory(board_fabric)
