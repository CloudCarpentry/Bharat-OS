cmake_minimum_required(VERSION 3.20)
project(BharatOS_UI_LCD C)

add_library(bharat_ui_lcd_tiny STATIC tiny_ui.c)
target_include_directories(bharat_ui_lcd_tiny PUBLIC
    ${CMAKE_SOURCE_DIR}/interface/include
)
