cmake_minimum_required(VERSION 3.20)
project(BharatOS_Services_RtSupervisor C ASM)

# Add the executable target for the rt-supervisor service
add_executable(rt-supervisor
    rt_supervisor.c
)
target_link_libraries(rt-supervisor PRIVATE bharat_user_buildopts bharat_freestanding bharat_syscall)
bharat_configure_userspace_binary(rt-supervisor NO_CRT0)
target_link_options(rt-supervisor PRIVATE -static)
