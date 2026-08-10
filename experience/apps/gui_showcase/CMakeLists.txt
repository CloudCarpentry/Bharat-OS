cmake_minimum_required(VERSION 3.20)

add_executable(gui_showcase
    main.c
)

target_include_directories(gui_showcase PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/stacks/ui/adapters/lvgl/include
    ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/third_party/lvgl/upstream
)

target_link_libraries(gui_showcase PRIVATE
    bharat_ui_lvgl_core bharat_freestanding
    bharat_freestanding
    bharat_libruntime
)

# Macro for BharatOS binaries (mocked for cmake targets if required)
if(COMMAND bharat_configure_userspace_binary)
    bharat_configure_userspace_binary(gui_showcase)
endif()
