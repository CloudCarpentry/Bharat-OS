# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

option(BHARAT_UI_HEADLESS_TESTS "Build LVGL memory-display tests" OFF)

if(BHARAT_UI_HEADLESS_TESTS)
    add_executable(bharat_ui_headless_test
        test_headless_render.c
    )

    target_link_libraries(bharat_ui_headless_test PRIVATE bharat_ui_lvgl_core)
endif()

if(NOT CMAKE_CROSSCOMPILING)
    add_executable(bharat_shell_model_test
        test_shell_model.c
        ${CMAKE_SOURCE_DIR}/experience/shell/shell_model.c
    )
    target_include_directories(bharat_shell_model_test PRIVATE ${CMAKE_SOURCE_DIR}/experience/shell)
    add_test(NAME bharat_shell_model_test COMMAND bharat_shell_model_test)
endif()
