add_library(hal_arm64_cpu OBJECT
    ../../kernel/src/hal/arm64/hal_cpu.c
    ../../kernel/src/hal/arm64/trap_entry.S
    ../../kernel/src/hal/arm64/vmm.c
    ../../kernel/src/arch/arm64/context_switch.c
    ../../kernel/src/arch/arm64/context_switch.S
    ../../kernel/src/arch/arm64/cpu_caps.c
    ../../kernel/src/arch/arm64/ext_state.c
    ../../kernel/src/hal/arm64/topology_fdt.c
)
# Ensure Clang knows to treat .S as arm64 assembly, avoiding errors when host is x86
set_source_files_properties(
    ../../kernel/src/hal/arm64/trap_entry.S
    ../../kernel/src/arch/arm64/context_switch.S
    PROPERTIES COMPILE_OPTIONS "-target;aarch64-unknown-none-elf"
)

add_library(hal_arm64_mmu OBJECT
    ../../kernel/src/hal/arm64/hal_pt_arm64.c
    ../../kernel/src/hal/hal_pt.c
    ../../kernel/src/hal/arm64/smmu.c
)

add_library(hal_arm64_irq OBJECT
    ../../kernel/src/hal/arm64/gicv3.c
)

add_library(hal_arm64_timer OBJECT
    ../../kernel/src/hal/arm64/timer_generic.c
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
