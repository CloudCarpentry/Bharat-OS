add_library(hal_common OBJECT
    ../common/fdt_parser.c
    ../common/discovery.c
    ../mmu_init.c
    ../iommu_stub.c
    ../interrupt_common.c
    ../timer_common.c
    ../common/power/null_power.c
    ../common/power/null_thermal.c
    ../common/memops/memops_fallback.c
)

target_include_directories(hal_common PRIVATE 
    ${CMAKE_SOURCE_DIR}/kernel/src
    ${CMAKE_SOURCE_DIR}/kernel/include 
    ${CMAKE_SOURCE_DIR}/kernel/include/generated 
    ${CMAKE_SOURCE_DIR}/drivers/include
    ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hal_common PRIVATE bharat_freestanding bharat_kernel_buildopts)

string(TOUPPER "${BHARAT_ARCH_FAMILY}" _bharat_arch_family_upper)
if(BHARAT_ENABLE_DRIVER_ARCH_IRQ_TIMER AND (_bharat_arch_family_upper STREQUAL "X86"
   OR _bharat_arch_family_upper STREQUAL "ARM"
   OR _bharat_arch_family_upper STREQUAL "RISCV"))
    target_sources(hal_common PRIVATE arch_irq_timer/arch_irq_timer.c)
    target_compile_definitions(hal_common PRIVATE BHARAT_GENERIC_ENABLE_ARCH_IRQ_TIMER)
endif()
