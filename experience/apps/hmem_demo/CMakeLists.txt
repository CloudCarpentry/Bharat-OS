cmake_minimum_required(VERSION 3.20)
add_executable(hmem_demo main.c)
target_compile_features(hmem_demo PRIVATE c_std_11)
target_link_libraries(hmem_demo PRIVATE
    bharat_user_buildopts
    bharat_libcmin
    bharat_libruntime
    bharatc
    bharat_freestanding
)
if(COMMAND bharat_configure_userspace_binary)
    bharat_configure_userspace_binary(hmem_demo)
endif()
