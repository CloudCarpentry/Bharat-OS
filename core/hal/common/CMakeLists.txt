add_library(hal_common OBJECT
    ../common/fdt_parser.c
    ../common/discovery.c
    ../common/cpu_features.c
    ../mmu_init.c
    ../interrupt_common.c
    ../timer_common.c
    iommu_adapter.c
    ../common/power/null_power.c
    ../common/power/null_thermal.c
    ../common/memops/mem_scalar.c
    ../common/hash/hash_fallback.c
)

target_include_directories(hal_common PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/hal/include
    ${CMAKE_SOURCE_DIR}/core/kernel/src
    ${CMAKE_SOURCE_DIR}/core/kernel/include 
    ${CMAKE_SOURCE_DIR}/core/kernel/include/generated 
    ${CMAKE_SOURCE_DIR}/core/drivers/include
    ${CMAKE_SOURCE_DIR}/interface/include)
target_link_libraries(hal_common PRIVATE bharat_freestanding bharat_kernel_buildopts)

# Apply targeted builtin suppression to the memory helper
set_source_files_properties(../common/memops/mem_scalar.c PROPERTIES COMPILE_OPTIONS "-fno-builtin-memset;-fno-builtin-memcpy;-fno-builtin-memmove;-fno-builtin-strcmp;-fno-builtin-strncmp;-fno-builtin-strlen;-fno-builtin-bzero")

string(TOUPPER "${BHARAT_ARCH_FAMILY}" _bharat_arch_family_upper)
if(BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER AND (_bharat_arch_family_upper STREQUAL "X86"
   OR _bharat_arch_family_upper STREQUAL "ARM"
   OR _bharat_arch_family_upper STREQUAL "RISCV"))
    target_sources(hal_common PRIVATE arch_irq_timer/arch_irq_timer.c)
    target_compile_definitions(hal_common PRIVATE BHARAT_GENERIC_ENABLE_ARCH_IRQ_TIMER)
endif()
