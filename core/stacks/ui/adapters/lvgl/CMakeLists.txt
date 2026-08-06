# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

option(BHARAT_UI_LVGL "Enable the LVGL adapter and UI core" OFF)

if(BHARAT_UI_LVGL)
    add_library(bharat_ui_lvgl_core STATIC)

    file(GLOB_RECURSE LVGL_SOURCES "${CMAKE_SOURCE_DIR}/third_party/lvgl/upstream/src/*.c")

    target_sources(bharat_ui_lvgl_core PRIVATE
        src/lvgl_display_adapter.c
        src/lvgl_input_adapter.c
        src/lvgl_tick_adapter.c
        src/lvgl_memory_adapter.c
        src/lvgl_theme_bharat.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/app.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens/splash_screen.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens/home_screen.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens/diagnostics_screen.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens/recovery_screen.c
        ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens/settings_screen.c
        ${CMAKE_SOURCE_DIR}/build/generated/ui/theme/theme_tokens.c
        ${LVGL_SOURCES}
    )

    target_include_directories(bharat_ui_lvgl_core
        PUBLIC include
        PUBLIC ${CMAKE_SOURCE_DIR}/core/apps/ui_demo/screens
        PUBLIC ${CMAKE_SOURCE_DIR}/build/generated/ui/theme
        PRIVATE config
        PUBLIC ${CMAKE_SOURCE_DIR}/third_party/lvgl/upstream
    )

    target_compile_definitions(bharat_ui_lvgl_core PUBLIC LV_CONF_INCLUDE_SIMPLE=1)
    target_compile_definitions(bharat_ui_lvgl_core PUBLIC BHARAT_UI_LVGL)
    target_compile_definitions(bharat_ui_lvgl_core PRIVATE "LV_CONF_PATH=${CMAKE_SOURCE_DIR}/core/stacks/ui/adapters/lvgl/config/lv_conf_bharat.h")
endif()
