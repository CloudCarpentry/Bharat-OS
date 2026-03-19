add_library(hal_riscv64_cpu OBJECT
    ../../kernel/src/hal/riscv/hal_cpu.c
    ../../kernel/src/hal/riscv/boot/feature_probe.c
    ../../kernel/src/hal/riscv/trap_entry.S
    ../../kernel/src/hal/riscv/vmm.c
    ../../kernel/src/arch/riscv64/context_switch.c
    ../../kernel/src/arch/riscv64/context_switch.S
    ../../kernel/src/arch/riscv64/cpu_caps.c
    ../../kernel/src/arch/riscv64/ext_state.c
    ../../kernel/src/hal/riscv/shakti_bsp.c
)

add_library(hal_riscv64_mmu OBJECT
    ../../kernel/src/hal/riscv64/hal_pt_riscv64.c
    ../../kernel/src/hal/hal_pt.c
    ../../kernel/src/hal/riscv/iopmp.c
)

add_library(hal_riscv64_irq OBJECT
    ../../kernel/src/hal/riscv/plic.c
)

add_library(hal_riscv64_timer OBJECT
    ../../kernel/src/hal/riscv/sbi_timer_ipi.c
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
