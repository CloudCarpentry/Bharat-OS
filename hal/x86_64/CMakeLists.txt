add_library(hal_x86_64_cpu OBJECT
    ../../kernel/src/hal/x86_64/hal_cpu.c
    ../../kernel/src/hal/x86_64/trap_entry.S
    ../../kernel/src/hal/x86_64/vmm.c
    ../../kernel/src/arch/x86_64/context_switch.c
    ../../kernel/src/arch/x86_64/context_switch.S
    ../../kernel/src/arch/x86_64/cpu_caps.c
    ../../kernel/src/arch/x86_64/ext_state.c
    ../../kernel/src/arch/capabilities.c
)

add_library(hal_x86_64_mmu OBJECT
    ../../kernel/src/hal/x86_64/hal_pt_x86_64.c
    ../../kernel/src/hal/hal_pt.c
    ../../kernel/src/hal/x86_64/vtd_iommu.c
)

add_library(hal_x86_64_irq OBJECT
    ../../kernel/src/hal/x86_64/apic.c
)

add_library(hal_x86_64_timer OBJECT
    ../../kernel/src/hal/x86_64/timer.c
)

target_include_directories(hal_x86_64_cpu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_mmu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_irq PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_timer PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)

target_link_libraries(hal_x86_64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)
