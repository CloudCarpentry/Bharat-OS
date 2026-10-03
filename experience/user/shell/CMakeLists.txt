cmake_minimum_required(VERSION 3.20)

project(bh_shell C)

add_executable(execplan execplan.c)
target_include_directories(execplan PRIVATE ../../../interface/include ../../../interface/uapi)
target_link_libraries(execplan PUBLIC bharat_libcmin bharat_user_buildopts bharat bharat_syscall bharat_libruntime)

bharat_configure_userspace_binary(execplan)
