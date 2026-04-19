cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_IPC C ASM)

# Add the lib/ipc static library
add_library(bharat_libipc STATIC src/ipc.c)

# Set the public include directory
target_include_directories(bharat_libipc PUBLIC include ../include ../posix/include)

# IPC depends on cap headers
target_link_libraries(bharat_libipc PUBLIC cap bharat_freestanding bharat_syscall bharat_libcmin bharat_libstring bharat_libruntime)
bharat_configure_userspace_library(bharat_libipc)

# Alias for compatibility
add_library(ipc ALIAS bharat_libipc)
