add_library(hal_riscv64_cpu OBJECT
    ../hal_cpu.c
    ../boot/feature_probe.c
    ../trap_entry.S
    ../../mmu/riscv64/vmm.c
    ../../../../arch/riscv/riscv64/context_switch.c
    ../../../../arch/riscv/riscv64/context_switch.S
    ../../../../arch/riscv/riscv64/cpu_caps.c
    ../../../../arch/riscv/riscv64/ext_state.c
    ../shakti_bsp.c
    hash/hash_riscv64.c
    cpu_relax.c
    boot_sbi.c
    topology_fdt.c
)

add_library(hal_riscv64_mmu OBJECT
    hal_pt_riscv64.c
    ../../hal_pt.c
    ../iopmp.c
    hal_mm.c
)

add_library(hal_riscv64_irq OBJECT
    ../hal_irq.c
)

add_library(hal_riscv64_timer OBJECT
    ../hal_timer.c
)

target_include_directories(hal_riscv64_cpu PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_riscv64_mmu PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_riscv64_irq PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)
target_include_directories(hal_riscv64_timer PRIVATE ../../../kernel/include ../../../kernel/include/generated ../../../include)

target_link_libraries(hal_riscv64_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_irq PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_riscv64_timer PRIVATE bharat_freestanding bharat_kernel_buildopts)

target_compile_options(hal_riscv64_cpu PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_mmu PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_irq PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
target_compile_options(hal_riscv64_timer PRIVATE "-target" "riscv64-unknown-none-elf" "-march=rv64gc")
