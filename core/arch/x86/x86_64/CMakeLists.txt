add_library(hal_x86_64_cpu OBJECT
    hal_cpu.c
    boot_ap.c
    hal_discovery.c
    trap_entry.S
    user_entry.c
    context_switch.c
    context_switch.S
    cpu_caps.c
    ext_state.c
    topology_acpi.c
    hash/hash_x86_64.c
    cpu_relax.c
)

add_library(hal_x86_64_mmu OBJECT
    hal_pt_x86_64.c
    ../../../hal/common/hal_pt.c
    vtd_iommu.c
    hal_mm.c
)

add_library(hal_x86_64_irq OBJECT
    hal_irq.c
)

add_library(hal_x86_64_timer OBJECT
    hal_timer.c
)

target_include_directories(hal_x86_64_cpu PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_x86_64_mmu PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_x86_64_irq PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_x86_64_timer PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)

target_link_libraries(hal_x86_64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_x86_64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)

