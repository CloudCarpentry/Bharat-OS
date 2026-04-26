add_library(k_trap OBJECT
    ../../src/trap/trap.c
    ../../src/trap/syscall.c
    ../../src/trap/syscall_gate.c
    ../../src/trap/usercopy.c
    ../../src/trap/syscall_stats.c
    ../../src/trap/trap_fault.c
    ../../src/fault_diag.c
    $<$<STREQUAL:${ARCH},x86_64>:${CMAKE_SOURCE_DIR}/core/arch/x86/x86_64/syscall_extract.c>
    $<$<STREQUAL:${ARCH},arm64>:${CMAKE_SOURCE_DIR}/core/arch/arm/arm64/syscall_extract.c>
    $<$<STREQUAL:${ARCH},riscv64>:${CMAKE_SOURCE_DIR}/core/arch/riscv/riscv64/syscall_extract.c>
)

target_include_directories(k_trap PRIVATE ../../include ${BHARAT_GENERATED_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/interface/include ${CMAKE_SOURCE_DIR}/core/lib/include ${BHARAT_BOOT_INCLUDE_DIR})
target_link_libraries(k_trap PRIVATE bharat_freestanding bharat_kernel_buildopts)
