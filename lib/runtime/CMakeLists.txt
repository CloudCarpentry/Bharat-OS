cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_Runtime C ASM)

# Create crt0 as an OBJECT library so it can be explicitly linked into every binary
add_library(bharat_crt0 OBJECT src/crt0.c)
target_include_directories(bharat_crt0 PUBLIC include)
target_link_libraries(bharat_crt0 PUBLIC bharat_freestanding bharat_syscall)

# Add the lib/runtime static library
add_library(bharat_libruntime STATIC src/runtime.c src/freestanding_string.c)

# Set the public include directory
target_include_directories(bharat_libruntime PUBLIC include)

# Runtime depends on cap and ipc headers
target_link_libraries(bharat_libruntime PUBLIC cap ipc bharat_syscall bharat_freestanding)
bharat_configure_userspace_library(bharat_libruntime)

# Alias for compatibility
add_library(runtime ALIAS bharat_libruntime)
