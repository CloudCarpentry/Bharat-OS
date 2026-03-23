add_executable(app_test main.c arch_wrappers.c runner/test_runner.c pmm/pmm_stress.c vmm/vmm_stress.c)
target_link_libraries(app_test PUBLIC bharat_libcmin bharat_user_buildopts bharat_syscall bharat)
bharat_configure_userspace_binary(app_test)
