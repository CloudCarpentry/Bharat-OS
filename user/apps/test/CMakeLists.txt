add_executable(app_test
    main.c
    arch_wrappers.c
    runner/test_runner.c
    pmm/pmm_stress.c
    vmm/vmm_stress.c
)
# Force PIC/PIE off for tests as they have static state that trips up PIE
target_compile_options(app_test PRIVATE -fno-pie -fno-PIC)
target_link_options(app_test PRIVATE -Wl,-no-pie)

target_link_libraries(app_test PRIVATE bharat_user_buildopts bharat_freestanding bharat_libcmin bharat bharat_syscall)
