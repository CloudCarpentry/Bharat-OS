add_executable(drivers main.c)
    target_link_libraries(drivers PRIVATE bharat_user_buildopts bharat_freestanding)
target_include_directories(drivers PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)

target_link_libraries(drivers PRIVATE urpc subsys_manager)
