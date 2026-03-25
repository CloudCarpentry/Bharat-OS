add_library(hal_riscv64_cpu OBJECT
    ../riscv/hal_cpu.c
    ../riscv/boot/feature_probe.c
    ../riscv/trap_entry.S
    ../mmu/riscv64/vmm.c
    ../arch/riscv64/context_switch.c
    ../arch/riscv64/context_switch.S
    ../arch/riscv64/cpu_caps.c
    ../arch/riscv64/ext_state.c
    ../riscv/shakti_bsp.c
    ../riscv64/hash/hash_riscv64.c
)

add_library(hal_riscv64_mmu OBJECT
    ../riscv64/hal_pt_riscv64.c
    ../hal_pt.c
    ../riscv/iopmp.c
)

add_library(hal_riscv64_irq OBJECT
    ../riscv/plic.c
)

add_library(hal_riscv64_timer OBJECT
    ../riscv/sbi_timer_ipi.c
)

target_include_directories(hal_riscv64_cpu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_riscv64_mmu PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_riscv64_irq PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)
target_include_directories(hal_riscv64_timer PRIVATE ../../kernel/include ../../kernel/include/generated ../../include)

target_link_libraries(hal_riscv64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)

target_compile_options(hal_riscv64_cpu PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_mmu PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_irq PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_timer PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
