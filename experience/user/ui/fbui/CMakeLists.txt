# Minimal fbui target definition
set(BHARAT_FBUI_APP_SOURCE core/fb_demo_app.c)

if(BHARAT_INPUT_SHOWCASE_GUI)
    set(BHARAT_FBUI_APP_SOURCE input/fb_input_showcase.c)
    add_compile_definitions(BHARAT_INPUT_SHOWCASE_GUI=1)
endif()

add_library(fbui STATIC
    core/fb_render.c
    core/fb_events.c
    widgets/fb_widgets.c
    input/fbui_input_bridge.c
    ${BHARAT_FBUI_APP_SOURCE}
)
target_include_directories(fbui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fbui PRIVATE
    ${CMAKE_SOURCE_DIR}/core/kernel/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/core/drivers/input/include
    ${CMAKE_SOURCE_DIR}/core/drivers/input/virtio_input
    ${CMAKE_SOURCE_DIR}/core/drivers/virtio/core
    ${CMAKE_SOURCE_DIR}/core/drivers/virtio/pci
    ${CMAKE_SOURCE_DIR}/core/services/device/inputmgr/include
)
# Depend narrowly on the user library context
target_link_libraries(fbui PUBLIC bharat_user_buildopts bharat_freestanding)
