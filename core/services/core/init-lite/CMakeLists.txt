add_executable(init-lite init_lite.c)
target_link_libraries(init-lite PRIVATE bharat_user_buildopts bharat_freestanding
    bharat_libcmin bharat_libruntime bharat_syscall)
bharat_configure_userspace_binary(init-lite)
