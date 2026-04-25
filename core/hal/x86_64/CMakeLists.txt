add_library(hal_x86_64_cpu OBJECT
    ../x86_64/hal_cpu.c
    ../x86_64/trap_entry.S
    ../mmu/x86_64/vmm.c
    ../arch/x86_64/context_switch.c
    ../arch/x86_64/context_switch.S
    ../arch/x86_64/cpu_caps.c
    ../arch/x86_64/ext_state.c
    ../../arch/x86/x86_64/hash/hash_x86_64_impl.c
)

add_library(hal_x86_64_mmu OBJECT
    ../x86_64/hal_pt_x86_64.c
    ../hal_pt.c
    ../x86_64/vtd_iommu.c
)

add_library(hal_x86_64_irq OBJECT
    ../x86_64/apic.c
)

add_library(hal_x86_64_timer OBJECT
    ../x86_64/timer.c
)

target_include_directories(hal_x86_64_cpu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_mmu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_irq PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_x86_64_timer PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)

target_link_libraries(hal_x86_64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)
