cmake_minimum_required(VERSION 3.20)

add_executable(gui_showcase
    display_client.c
    input_client.c
    main.c
)

target_include_directories(gui_showcase PRIVATE
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/stacks/ui/adapters/lvgl/include
    ${CMAKE_SOURCE_DIR}/experience/shell
    ${CMAKE_SOURCE_DIR}/core/lib/include
    ${CMAKE_SOURCE_DIR}/core/lib/ipc/include
    ${CMAKE_SOURCE_DIR}/core/lib/namesvc/include
    ${CMAKE_SOURCE_DIR}/third_party/lvgl/upstream
    ${CMAKE_SOURCE_DIR}/third_party/lvgl/upstream/src
)

target_link_libraries(gui_showcase PRIVATE
    bharat_ui_lvgl_core
    bharat_user_buildopts
    bharat_libcmin
    bharat_libruntime
    bharat_libipc
    bharat_libnamesvc
    bharatc
    bharat_freestanding
)

target_compile_definitions(gui_showcase PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    "LV_CONF_PATH=\"${CMAKE_SOURCE_DIR}/core/stacks/ui/adapters/lvgl/config/lv_conf_bharat.h\""
)

# Macro for BharatOS binaries (mocked for cmake targets if required)
if(COMMAND bharat_configure_userspace_binary)
    bharat_configure_userspace_binary(gui_showcase)
endif()
