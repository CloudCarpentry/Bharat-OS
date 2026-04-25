add_library(hal_arm64_cpu OBJECT
    ../arm64/hal_cpu.c
    ../arm64/trap_entry.S
    ../mmu/arm64/vmm.c
    ../arch/arm64/context_switch.c
    ../arch/arm64/context_switch.S
    ../arch/arm64/cpu_caps.c
    ../arch/arm64/ext_state.c
    ../arm64/topology_fdt.c
    ../arm64/hash/hash_arm64.c
)
# Ensure Clang knows to treat .S as arm64 assembly, avoiding errors when host is x86
set_source_files_properties(
    ../arm64/trap_entry.S
    ../arch/arm64/context_switch.S
    PROPERTIES COMPILE_OPTIONS "-target;aarch64-unknown-none-elf"
)

add_library(hal_arm64_mmu OBJECT
    ../arm64/hal_pt_arm64.c
    ../hal_pt.c
    ../arm64/smmu.c
)

add_library(hal_arm64_irq OBJECT
    ../arm64/gicv3.c
)

add_library(hal_arm64_timer OBJECT
    ../arm64/timer_generic.c
)

target_include_directories(hal_arm64_cpu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_arm64_mmu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_arm64_irq PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_arm64_timer PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)

target_link_libraries(hal_arm64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_arm64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_arm64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_arm64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)

# When cross-compiling or building host targets, assure arm64 code is compiled with arm64 target
target_compile_options(hal_arm64_cpu PRIVATE "-target" "aarch64-unknown-none-elf")
target_compile_options(hal_arm64_mmu PRIVATE "-target" "aarch64-unknown-none-elf")
target_compile_options(hal_arm64_irq PRIVATE "-target" "aarch64-unknown-none-elf")
target_compile_options(hal_arm64_timer PRIVATE "-target" "aarch64-unknown-none-elf")
