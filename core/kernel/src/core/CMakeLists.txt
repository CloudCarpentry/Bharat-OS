add_library(k_core OBJECT
    ${BHARAT_KERNEL_SRC_ROOT}/cpu_local.c
    $<$<STREQUAL:${ARCH},x86_64>:${BHARAT_ARCH_ROOT}/x86/x86_64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},arm64>:${BHARAT_ARCH_ROOT}/arm/arm64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},riscv64>:${BHARAT_ARCH_ROOT}/riscv/riscv64/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},arm32>:${BHARAT_ARCH_ROOT}/arm/arm32/kernel/cpu_local.c>
    $<$<STREQUAL:${ARCH},riscv32>:${BHARAT_ARCH_ROOT}/riscv/riscv32/kernel/cpu_local.c>
    ${BHARAT_KERNEL_SRC_ROOT}/multicore.c
    ${BHARAT_KERNEL_SRC_ROOT}/subsystem/profile.c
    ${BHARAT_KERNEL_SRC_ROOT}/profile/profile.c
    ../profile/profile_policy.c
    ${BHARAT_KERNEL_STAGING_ROOT}/ai/ai_kernel_bridge.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/isolation.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/credentials.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/policy.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/audit.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/secure_boot.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/vfio.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/crypto_registry.c
    ${BHARAT_KERNEL_SRC_ROOT}/security/crypto_caps.c
    ${BHARAT_KERNEL_SRC_ROOT}/lib/base/string.c
    ${BHARAT_ARCH_ROOT}/common/memops_scalar.c
    $<$<STREQUAL:${ARCH},x86_64>:${BHARAT_ARCH_ROOT}/x86/x86_64/memops.c>
    $<$<STREQUAL:${ARCH},x86_64>:${BHARAT_ARCH_ROOT}/x86/x86_64/memops_fast_string.c>
    $<$<STREQUAL:${ARCH},arm32>:${BHARAT_ARCH_ROOT}/arm/arm32/memops.c>
    $<$<STREQUAL:${ARCH},arm64>:${BHARAT_ARCH_ROOT}/arm/arm64/memops.c>
    $<$<STREQUAL:${ARCH},arm64>:${BHARAT_ARCH_ROOT}/arm/arm64/memops_gpr_bulk.c>
    $<$<STREQUAL:${ARCH},riscv64>:${BHARAT_ARCH_ROOT}/riscv/riscv64/memops.c>
    $<$<STREQUAL:${ARCH},riscv64>:${BHARAT_ARCH_ROOT}/riscv/riscv64/memops_gpr_bulk.c>
    $<$<STREQUAL:${ARCH},riscv32>:${BHARAT_ARCH_ROOT}/riscv/riscv64/memops.c>
    $<$<STREQUAL:${ARCH},riscv32>:${BHARAT_ARCH_ROOT}/riscv/riscv64/memops_gpr_bulk.c>
    ${BHARAT_KERNEL_SRC_ROOT}/stack_protector.c
    ${BHARAT_KERNEL_SRC_ROOT}/crypto/secure_mem.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/power_core.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/power_domain.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/thermal_core.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/thermal_trip.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/cooling_device.c
    ${BHARAT_KERNEL_SRC_ROOT}/power/power_qos.c
    hw_caps_registry.c
)

target_include_directories(k_core PRIVATE
    ${BHARAT_KERNEL_INCLUDE_DIR}
    ${BHARAT_GENERATED_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/core/personalities/runtime/include
    ${CMAKE_SOURCE_DIR}/interface/include
    ${BHARAT_LIB_ROOT}/include
    ${BHARAT_BOOT_INCLUDE_DIR}
)
target_link_libraries(k_core PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_sources(k_core PRIVATE fault_domain.c)
