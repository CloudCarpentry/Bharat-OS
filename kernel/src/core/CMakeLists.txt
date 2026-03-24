add_library(k_core OBJECT
    ../../src/cpu_local.c
    ../../src/multicore.c
    ../../src/subsystem/profile.c
    ../../src/profile/profile.c
    ../../staging/ai/ai_kernel_bridge.c
    ../../src/security/isolation.c
    ../../src/security/credentials.c
    ../../src/security/policy.c
    ../../src/security/audit.c
    ../../src/security/secure_boot.c
    ../../src/security/vfio.c
    ../../src/lib/string.c
    ../../../arch/common/memops_scalar.c
    $<$<STREQUAL:${ARCH},x86_64>:../../../arch/x86_64/memops.c>
    $<$<STREQUAL:${ARCH},x86_64>:../../../arch/x86_64/memops_fast_string.c>
    $<$<STREQUAL:${ARCH},arm64>:../../../arch/arm64/memops.c>
    $<$<STREQUAL:${ARCH},arm64>:../../../arch/arm64/memops_gpr_bulk.c>
    $<$<STREQUAL:${ARCH},riscv64>:../../../arch/riscv64/memops.c>
    $<$<STREQUAL:${ARCH},riscv64>:../../../arch/riscv64/memops_gpr_bulk.c>
    ../../src/stack_protector.c
    ../../src/crypto/secure_mem.c
    ../../src/power/power_core.c
    ../../src/power/power_domain.c
    ../../src/power/thermal_core.c
    ../../src/power/thermal_trip.c
    ../../src/power/cooling_device.c
    ../../src/power/power_qos.c
)

target_include_directories(k_core PRIVATE ../../include ../../include/generated ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(k_core PRIVATE bharat_freestanding bharat_kernel_buildopts)
