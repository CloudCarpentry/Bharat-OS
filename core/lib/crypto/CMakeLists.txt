# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)

# We define the options to avoid building testing/programs from tf-psa-crypto
set(TF_PSA_CRYPTO_TESTING OFF CACHE BOOL "Disable tests from tf-psa-crypto" FORCE)
set(TF_PSA_CRYPTO_PROGRAMS OFF CACHE BOOL "Disable programs from tf-psa-crypto" FORCE)
set(TF_PSA_CRYPTO_MAKE_DEFS OFF CACHE BOOL "Disable automatic generation via make" FORCE)

# Setup path to our custom config
set(TF_PSA_CRYPTO_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config/bharat_crypto_config.h" CACHE STRING "Configuration file" FORCE)

# We will build TF-PSA-Crypto ourselves rather than relying on its CMake to configure the library target properly.
add_library(tfpsacrypto STATIC)

# Generate PSA Driver Wrappers and config
set(TF_PSA_CRYPTO_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/tf-psa-crypto/upstream")
set(TF_PSA_CRYPTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/tf_psa_crypto")

add_custom_command(
    OUTPUT
        ${TF_PSA_CRYPTO_GEN_DIR}/psa_crypto_driver_wrappers.h
        ${TF_PSA_CRYPTO_GEN_DIR}/psa_crypto_driver_wrappers_no_static.c
        ${TF_PSA_CRYPTO_GEN_DIR}/tf_psa_crypto_config_check_before.h
        ${TF_PSA_CRYPTO_GEN_DIR}/tf_psa_crypto_config_check_after.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TF_PSA_CRYPTO_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${TF_PSA_CRYPTO_SRC_DIR}/scripts:${CMAKE_SOURCE_DIR}/third_party/tf-psa-crypto/upstream/framework/scripts python3 ${TF_PSA_CRYPTO_SRC_DIR}/scripts/generate_driver_wrappers.py
        ${TF_PSA_CRYPTO_GEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${TF_PSA_CRYPTO_SRC_DIR}/scripts:${CMAKE_SOURCE_DIR}/third_party/tf-psa-crypto/upstream/framework/scripts python3 ${TF_PSA_CRYPTO_SRC_DIR}/scripts/generate_config_checks.py
        ${TF_PSA_CRYPTO_GEN_DIR}
    DEPENDS
        ${TF_PSA_CRYPTO_SRC_DIR}/scripts/generate_driver_wrappers.py
        ${TF_PSA_CRYPTO_SRC_DIR}/scripts/generate_config_checks.py
        ${TF_PSA_CRYPTO_SRC_DIR}/scripts/data_files/driver_templates/psa_crypto_driver_wrappers.h.jinja
        ${TF_PSA_CRYPTO_SRC_DIR}/scripts/data_files/driver_templates/psa_crypto_driver_wrappers_no_static.c.jinja
)

target_include_directories(tfpsacrypto PUBLIC
    include
    ${TF_PSA_CRYPTO_SRC_DIR}/include
    ${TF_PSA_CRYPTO_SRC_DIR}/drivers/builtin/include
)

# Start with empty sources
set(TF_PSA_CRYPTO_SOURCES "")

# Core PSA Framework
list(APPEND TF_PSA_CRYPTO_SOURCES
    "${TF_PSA_CRYPTO_SRC_DIR}/core/psa_crypto.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/psa_crypto_client.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/psa_crypto_random.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/psa_crypto_slot_management.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/psa_util.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/tf_psa_crypto_config.c"
    "${TF_PSA_CRYPTO_SRC_DIR}/core/tf_psa_crypto_version.c"
)

# Bring everything for now to fix undefined references - then we'll shrink it
file(GLOB TF_PSA_CRYPTO_DRIVERS "${TF_PSA_CRYPTO_SRC_DIR}/drivers/builtin/src/*.c")
list(APPEND TF_PSA_CRYPTO_SOURCES ${TF_PSA_CRYPTO_DRIVERS})

# Bring extras
file(GLOB TF_PSA_CRYPTO_EXTRAS "${TF_PSA_CRYPTO_SRC_DIR}/extras/*.c")
list(APPEND TF_PSA_CRYPTO_SOURCES ${TF_PSA_CRYPTO_EXTRAS})

# Utilities & Platform
file(GLOB TF_PSA_CRYPTO_UTILS "${TF_PSA_CRYPTO_SRC_DIR}/utilities/*.c")
# EXCLUDE PEM
list(REMOVE_ITEM TF_PSA_CRYPTO_UTILS "${TF_PSA_CRYPTO_SRC_DIR}/utilities/pem.c")

list(APPEND TF_PSA_CRYPTO_SOURCES ${TF_PSA_CRYPTO_UTILS})

target_sources(tfpsacrypto PRIVATE
    ${TF_PSA_CRYPTO_SOURCES}
    src/bharat_entropy.c
    src/bharat_memory.c
    ${TF_PSA_CRYPTO_GEN_DIR}/psa_crypto_driver_wrappers_no_static.c
)

# We will need to configure PSA definitions. Let's add config directory
target_include_directories(tfpsacrypto PRIVATE
    config
    ${TF_PSA_CRYPTO_SRC_DIR}/core
    ${TF_PSA_CRYPTO_SRC_DIR}/dispatch
    ${TF_PSA_CRYPTO_SRC_DIR}/platform
    ${TF_PSA_CRYPTO_SRC_DIR}/utilities
    ${TF_PSA_CRYPTO_SRC_DIR}/extras
    ${TF_PSA_CRYPTO_SRC_DIR}/drivers/builtin/src
    ${TF_PSA_CRYPTO_GEN_DIR}
)

target_compile_definitions(tfpsacrypto PUBLIC
    TF_PSA_CRYPTO_USER_CONFIG_FILE=\"bharat_crypto_config.h\"
)

# We are building for freestanding environments. Avoid including std C headers from system.
target_compile_definitions(tfpsacrypto PRIVATE
    MBEDTLS_PLATFORM_MEMORY
    MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
    MBEDTLS_PLATFORM_STD_CALLOC=mbedtls_platform_calloc_macro
    MBEDTLS_PLATFORM_STD_FREE=mbedtls_platform_free_macro
)

target_link_libraries(tfpsacrypto PUBLIC
    bharat_freestanding
    bharatc
)

if (COMMAND bharat_configure_userspace_library)
    bharat_configure_userspace_library(tfpsacrypto)
endif()

# The target provided by TF-PSA-Crypto is usually 'tfpsacrypto'
# We will create an ALIAS to match our naming scheme, or link it.
add_library(bharat_libcrypto ALIAS tfpsacrypto)

add_executable(crypto_tests tests/crypto_tests.c tests/test_stubs.c)
target_include_directories(crypto_tests PUBLIC
    config
    ${CMAKE_SOURCE_DIR}/interface/include
    ${CMAKE_SOURCE_DIR}/third_party/tf-psa-crypto/upstream/include
)

target_link_libraries(crypto_tests PUBLIC tfpsacrypto bharatc bharat_freestanding bharatsys)

if (COMMAND bharat_configure_userspace_binary)
    bharat_configure_userspace_binary(crypto_tests)
endif()
