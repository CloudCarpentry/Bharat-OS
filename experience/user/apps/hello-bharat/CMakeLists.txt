# Bharat-OS SDK Native Application Template
# -----------------------------------------
# This is a reusable CMake template for building native applications using the Bharat-OS toolchain.

if(NOT TARGET bharat_libruntime)
    message(FATAL_ERROR "Bharat native runtime target 'bharat_libruntime' is unavailable")
endif()

if(NOT TARGET bharat_syscall)
    message(FATAL_ERROR "Bharat syscall backend 'bharat_syscall' is unavailable")
endif()

if(NOT TARGET bharat_crt0)
    message(FATAL_ERROR "Bharat userspace startup object 'bharat_crt0' is unavailable")
endif()

if(NOT COMMAND bharat_configure_userspace_binary)
    message(FATAL_ERROR "bharat_configure_userspace_binary() is unavailable")
endif()

add_executable(app_hello_bharat main.c)

target_compile_features(app_hello_bharat PRIVATE c_std_11)

target_compile_options(app_hello_bharat PRIVATE
    -Wall
    -Wextra
    -Werror=implicit-function-declaration
)

# Link against the basic freestanding environment and runtime libraries
target_link_libraries(app_hello_bharat PRIVATE
    bharat_user_buildopts
    bharat_freestanding
    bharat_libruntime
)

# Apply Bharat-OS specific application settings (PIE, crt0 injection, linker flags)
bharat_configure_userspace_binary(app_hello_bharat)
