add_library(subsys_manager STATIC
    manager.c
    subsys_test_runner.c
)
target_include_directories(subsys_manager PUBLIC ${CMAKE_SOURCE_DIR}/kernel/include ${CMAKE_SOURCE_DIR}/subsys/include)
target_link_libraries(subsys_manager PUBLIC bharat_user_buildopts bharat_freestanding bharat_generated_includes)
