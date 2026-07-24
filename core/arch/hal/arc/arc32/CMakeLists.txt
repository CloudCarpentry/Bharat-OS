add_library(hal_arc32_cpu OBJECT
    hal_cpu.c hal_discovery.c hal_irq.c hal_timer.c
    ../../../arch/arc/arc32/context_switch.c
    ../../../arch/arc/arc32/boot_stub.S
)

add_library(hal_arc32_mmu OBJECT
    hal_mm.c
)

target_include_directories(hal_arc32_cpu PRIVATE ../../../kernel/include ../../../include)
target_include_directories(hal_arc32_mmu PRIVATE ../../../kernel/include ../../../include)

target_link_libraries(hal_arc32_cpu PRIVATE bharat_freestanding bharat_kernel_buildopts)
target_link_libraries(hal_arc32_mmu PRIVATE bharat_freestanding bharat_kernel_buildopts)

# ARC specific options
target_compile_options(hal_arc32_cpu PRIVATE "-target" "arc-unknown-elf" "-mcpu=hs38")
target_compile_options(hal_arc32_mmu PRIVATE "-target" "arc-unknown-elf" "-mcpu=hs38")
