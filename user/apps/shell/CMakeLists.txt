add_executable(app_shell shell.c)
target_compile_options(app_shell PRIVATE -fno-pie -fno-PIC)
target_link_options(app_shell PRIVATE -Wl,-no-pie)
target_link_libraries(app_shell PRIVATE bharat_user_buildopts bharat_freestanding bharat_libcmin bharat bharat_syscall)
