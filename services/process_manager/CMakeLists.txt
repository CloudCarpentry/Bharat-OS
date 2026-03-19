add_executable(process_manager main.c)
    target_link_libraries(process_manager PRIVATE bharat_user_buildopts bharat_freestanding)
target_include_directories(process_manager PRIVATE ../../kernel/include ../../subsys/include ../../lib/urpc)

target_link_libraries(process_manager PRIVATE urpc subsys_manager)
