add_library(k_core OBJECT
    ${CMAKE_SOURCE_DIR}/kernel/src/cpu_local.c
    $<$<STREQUAL:${ARCH},x86_64>:${CMAKE_SOURCE_DIR}/arch/x86/x86_64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},arm64>:${CMAKE_SOURCE_DIR}/arch/arm/arm64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},riscv64>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},arm32>:${CMAKE_SOURCE_DIR}/arch/arm/arm32/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},riscv32>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv32/kernel/cpu_local.c>
    ${CMAKE_SOURCE_DIR}/kernel/src/multicore.c
    ${CMAKE_SOURCE_DIR}/kernel/src/subsystem/profile.c
    ${CMAKE_SOURCE_DIR}/kernel/src/profile/profile.c
    ${BHARAT_KERNEL_STAGING_ROOT}/ai/ai_kernel_bridge.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/isolation.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/credentials.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/policy.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/audit.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/secure_boot.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/vfio.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/crypto_registry.c
    ${CMAKE_SOURCE_DIR}/kernel/src/security/crypto_caps.c
    ${CMAKE_SOURCE_DIR}/kernel/src/lib/base/string.c
    ${CMAKE_SOURCE_DIR}/arch/common/memops_scalar.c
    $<$<STREQUAL:${ARCH},x86_64>:${CMAKE_SOURCE_DIR}/arch/x86/x86_64/memops.c>
    $<$<STREQUAL:${ARCH},x86_64>:${CMAKE_SOURCE_DIR}/arch/x86/x86_64/memops_fast_string.c>
    $<$<STREQUAL:${ARCH},arm32>:${CMAKE_SOURCE_DIR}/arch/arm/arm32/memops.c>
    $<$<STREQUAL:${ARCH},arm64>:${CMAKE_SOURCE_DIR}/arch/arm/arm64/memops.c>
    $<$<STREQUAL:${ARCH},arm64>:${CMAKE_SOURCE_DIR}/arch/arm/arm64/memops_gpr_bulk.c>
    $<$<STREQUAL:${ARCH},riscv64>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv64/memops.c>
    $<$<STREQUAL:${ARCH},riscv64>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv64/memops_gpr_bulk.c>
    $<$<STREQUAL:${ARCH},riscv32>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv64/memops.c>
    $<$<STREQUAL:${ARCH},riscv32>:${CMAKE_SOURCE_DIR}/arch/riscv/riscv64/memops_gpr_bulk.c>
    ${CMAKE_SOURCE_DIR}/kernel/src/stack_protector.c
    ${CMAKE_SOURCE_DIR}/kernel/src/crypto/secure_mem.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/power_core.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/power_domain.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/thermal_core.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/thermal_trip.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/cooling_device.c
    ${CMAKE_SOURCE_DIR}/kernel/src/power/power_qos.c
    hw_caps_registry.c
)

target_include_directories(k_core PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/kernel/include/generated
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/include
    ${BHARAT_BOOT_INCLUDE_DIR}
)
target_link_libraries(k_core PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_sources(k_core PRIVATE fault_domain.c)
