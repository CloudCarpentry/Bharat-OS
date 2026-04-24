add_executable(process_manager main.c process_manager.c)
target_link_libraries(process_manager PRIVATE bharat_user_buildopts bharat_freestanding bharat_libipc)
target_include_directories(process_manager PRIVATE ../../kernel/include ../../services/core/subsysmgr/include ../../lib/bharat_libtransport ../../include)

target_link_libraries(process_manager PRIVATE bharat_libtransport subsys_manager bharat_libstring)

bharat_configure_userspace_binary(process_manager)

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Skipping test_process_manager_caps build during cross-compilation")
else()
    add_executable(test_process_manager_caps tests/test_process_manager_caps.c process_manager.c)
    target_include_directories(test_process_manager_caps PRIVATE ../../kernel/include ../../services/core/subsysmgr/include ../../lib/bharat_libtransport ../../include ../../lib/cap/include ../../lib/ipc/include ../../../interface/include)
    target_link_libraries(test_process_manager_caps PRIVATE bharat_libcap_base)
    set_target_properties(test_process_manager_caps PROPERTIES COMPILE_OPTIONS "")
    set_target_properties(test_process_manager_caps PROPERTIES LINK_OPTIONS "")
    add_test(NAME test_process_manager_caps COMMAND test_process_manager_caps)
endif()
