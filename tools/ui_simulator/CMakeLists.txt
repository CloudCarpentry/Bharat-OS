# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

option(BHARAT_UI_SIMULATOR "Build the SDL host simulator" OFF)

if(BHARAT_UI_SIMULATOR)
    if(NOT BHARAT_UI_LVGL)
        message(FATAL_ERROR "BHARAT_UI_SIMULATOR requires BHARAT_UI_LVGL=ON")
    endif()

    find_package(SDL2 REQUIRED)

    add_executable(bharat_ui_simulator_sdl
        main.c
        simulator_platform.c
    )

    target_include_directories(bharat_ui_simulator_sdl PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(bharat_ui_simulator_sdl PRIVATE bharat_ui_lvgl_core ${SDL2_LIBRARIES})
    target_compile_definitions(bharat_ui_simulator_sdl PRIVATE "LV_CONF_PATH=${CMAKE_SOURCE_DIR}/tools/ui_simulator/lv_conf_simulator.h")
endif()
