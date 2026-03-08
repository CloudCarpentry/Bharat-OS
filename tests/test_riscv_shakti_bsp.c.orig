#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../kernel/include/hal/riscv_bsp.h"

static void assert_profile(const char* name,
                           riscv_soc_profile_t profile,
                           uint64_t expected_uart) {
    riscv_bsp_config_t cfg;
    assert(hal_riscv_bsp_detect(name, 0x88000000ULL, &cfg) == 0);
    assert(cfg.soc_profile == profile);
    assert(cfg.uart_base == expected_uart);
    assert(cfg.plic_base != 0ULL);
    assert(cfg.clint_base != 0ULL);
}

int main(void) {
    assert_profile("shakti-e", RISCV_SOC_SHAKTI_E, 0xF0000000ULL);
    assert_profile("shakti-c", RISCV_SOC_SHAKTI_C, 0xE0001800ULL);
    assert_profile("shakti-i", RISCV_SOC_SHAKTI_I, 0x20000000ULL);
    assert_profile("qemu-virt", RISCV_SOC_QEMU_VIRT, 0x10000000ULL);

    riscv_bsp_config_t active;
    assert(hal_riscv_bsp_detect("shakti-e", 0x81000000ULL, &active) == 0);
    assert(hal_riscv_bsp_init(&active) == 0);

    const riscv_bsp_config_t* got = hal_riscv_bsp_active();
    assert(got->soc_profile == RISCV_SOC_SHAKTI_E);
    assert(got->fdt_ptr == 0x81000000ULL);

    printf("RISC-V Shakti BSP tests passed.\n");
    return 0;
}
