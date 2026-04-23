add_executable(app_test main.c arch_wrappers.c runner/test_runner.c pmm/pmm_stress.c vmm/vmm_stress.c net_can/net_can_contract.c)
target_link_libraries(app_test PUBLIC bharat_libcmin bharat_user_buildopts bharat bharat_syscall)
bharat_configure_userspace_binary(app_test)

target_include_directories(app_test PRIVATE ${CMAKE_SOURCE_DIR}/drivers/include ${CMAKE_SOURCE_DIR}/stacks/include ${CMAKE_SOURCE_DIR}/lib/packet/include)
