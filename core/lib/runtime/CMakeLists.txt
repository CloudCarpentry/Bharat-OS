cmake_minimum_required(VERSION 3.20)
project(BharatOS_Lib_Runtime C ASM)

# Create crt0 as an OBJECT library so it can be explicitly linked into every binary
add_library(bharat_crt0 OBJECT src/crt0.c)
target_include_directories(bharat_crt0 PUBLIC include ${CMAKE_SOURCE_DIR}/core/lib)
target_link_libraries(bharat_crt0 PUBLIC bharat_freestanding bharat_syscall)

# Add the lib/runtime static library
add_library(bharat_libruntime STATIC src/runtime.c src/freestanding_string.c host/runtime_host.c)

# Add backend dispatch framework (Gated behind feature if needed, currently unconditionally available as an object/static)
add_library(bharat_runtime_backend_dispatch OBJECT backend_dispatch/backend_registry.c)
target_include_directories(bharat_runtime_backend_dispatch PRIVATE ${CMAKE_SOURCE_DIR}/interface)
target_link_libraries(bharat_runtime_backend_dispatch PUBLIC bharat_freestanding)
bharat_configure_userspace_library(bharat_runtime_backend_dispatch)

# Crypto Pilot
add_library(bharat_runtime_crypto OBJECT crypto/crypto_dispatch.c)
target_include_directories(bharat_runtime_crypto PRIVATE ${CMAKE_SOURCE_DIR}/interface)
target_link_libraries(bharat_runtime_crypto PUBLIC bharat_freestanding bharat_runtime_backend_dispatch)
bharat_configure_userspace_library(bharat_runtime_crypto)

# Tensor Pilot
add_library(bharat_runtime_tensor OBJECT accel/tensor_dispatch.c)
target_include_directories(bharat_runtime_tensor PRIVATE ${CMAKE_SOURCE_DIR}/interface)
target_link_libraries(bharat_runtime_tensor PUBLIC bharat_freestanding bharat_runtime_backend_dispatch)
bharat_configure_userspace_library(bharat_runtime_tensor)

# Set the public include directory
target_include_directories(bharat_libruntime PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/interface/uapi/runtime
    ${CMAKE_SOURCE_DIR}/core/kernel/include
)
target_link_libraries(bharat_libruntime PRIVATE bharat_runtime_backend_dispatch bharat_runtime_crypto bharat_runtime_tensor)

# Runtime depends on cap and ipc headers
target_link_libraries(bharat_libruntime PUBLIC cap ipc bharat_syscall bharat_freestanding)
bharat_configure_userspace_library(bharat_libruntime)

# Alias for compatibility
add_library(runtime ALIAS bharat_libruntime)
