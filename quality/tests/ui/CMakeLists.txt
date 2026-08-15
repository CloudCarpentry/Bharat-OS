# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

option(BHARAT_UI_HEADLESS_TESTS "Build LVGL memory-display tests" OFF)

if(BHARAT_UI_HEADLESS_TESTS)
    add_executable(bharat_ui_headless_test
        test_headless_render.c
    )

    target_link_libraries(bharat_ui_headless_test PRIVATE bharat_ui_lvgl_core)
endif()

add_executable(bharat_tiny_ui_test
    ../test_tiny_ui_stack.c
    ${CMAKE_SOURCE_DIR}/core/stacks/ui/lcd/tiny_ui.c
)
target_include_directories(bharat_tiny_ui_test PRIVATE ${CMAKE_SOURCE_DIR}/interface/include)
target_compile_options(bharat_tiny_ui_test PRIVATE -fPIC)
target_link_options(bharat_tiny_ui_test PRIVATE "LINKER:-no-pie" "-fno-pie")
target_link_libraries(bharat_tiny_ui_test PRIVATE bharat_user_buildopts bharat_freestanding bharat_libruntime)
bharat_configure_userspace_binary(bharat_tiny_ui_test)

