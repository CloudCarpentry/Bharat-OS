# SPDX-License-Identifier: MIT
add_library(bharat_ui_theme STATIC
    default_theme.c
)

target_include_directories(bharat_ui_theme PUBLIC
    ${CMAKE_SOURCE_DIR}/interface/include
)
