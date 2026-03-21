cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_Cap C ASM)

# Add the lib/cap static library
add_library(bharat_libcap_base STATIC src/cap.c)

# Set the public include directory
target_include_directories(bharat_libcap_base PUBLIC include)

# Keep the build freestanding and warning-clean using common targets
target_link_libraries(bharat_libcap_base PUBLIC bharat_freestanding bharat_kernel_buildopts)
bharat_configure_userspace_library(bharat_libcap_base)

# Alias old 'cap' for compatibility during migration
add_library(cap ALIAS bharat_libcap_base)
