cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_IPC C ASM)

# Add the lib/ipc static library
add_library(bharat_libipc STATIC src/ipc.c)

# Set the public include directory
target_include_directories(bharat_libipc PUBLIC include ${CMAKE_SOURCE_DIR}/core/lib/include ${CMAKE_SOURCE_DIR}/kernel/include)

# IPC depends on cap headers
target_link_libraries(bharat_libipc PUBLIC cap bharat_freestanding bharat_syscall)
bharat_configure_userspace_library(bharat_libipc)

# Alias for compatibility
add_library(ipc ALIAS bharat_libipc)
