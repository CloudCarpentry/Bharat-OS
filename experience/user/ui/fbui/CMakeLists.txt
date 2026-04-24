# Minimal fbui target definition
add_library(fbui STATIC
    core/fb_render.c
    core/fb_events.c
    core/fb_demo_app.c
    widgets/fb_widgets.c
)
target_include_directories(fbui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(fbui PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/include)
# Depend narrowly on the user library context
target_link_libraries(fbui PUBLIC bharat_user_buildopts bharat_freestanding)
