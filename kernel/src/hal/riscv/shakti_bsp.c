#include "hal/riscv_bsp.h"

#include <stddef.h>
#include <stdint.h>

static riscv_bsp_config_t g_active_cfg;

static int str_eq(const char* a, const char* b) {
    if (!a || !b) {
        return 0;
    }

    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static void cfg_clear(riscv_bsp_config_t* cfg) {
    if (!cfg) {
        return;
    }

    cfg->soc_profile = RISCV_SOC_QEMU_VIRT;
    cfg->fdt_ptr = 0ULL;
    cfg->uart_base = 0ULL;
    cfg->plic_base = 0ULL;
    cfg->clint_base = 0ULL;
    cfg->dram_base = 0ULL;
    cfg->dram_size = 0ULL;
}

int hal_riscv_bsp_detect(const char* profile, uint64_t fdt_ptr, riscv_bsp_config_t* out_cfg) {
    if (!profile || !out_cfg) {
        return -1;
    }

    cfg_clear(out_cfg);
    out_cfg->fdt_ptr = fdt_ptr;

    /*
     * Baseline board descriptors for early boot.
     * Runtime FDT parsing is planned to supersede these constants.
     */
    if (str_eq(profile, "shakti-e")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_E;
        out_cfg->uart_base = 0xF0000000ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x10000000ULL; /* 256 MiB baseline */
        return 0;
    }

    if (str_eq(profile, "shakti-c")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_C;
        out_cfg->uart_base = 0xE0001800ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x40000000ULL; /* 1 GiB baseline */
        return 0;
    }

    if (str_eq(profile, "shakti-i")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_I;
        out_cfg->uart_base = 0x20000000ULL;
        out_cfg->plic_base = 0x0C000000ULL;
        out_cfg->clint_base = 0x02000000ULL;
        out_cfg->dram_base = 0x80000000ULL;
        out_cfg->dram_size = 0x20000000ULL; /* 512 MiB baseline */
        return 0;
    }

    /* Default QEMU virt profile used for current runtime CI. */
    out_cfg->soc_profile = RISCV_SOC_QEMU_VIRT;
    out_cfg->uart_base = 0x10000000ULL;
    out_cfg->plic_base = 0x0C000000ULL;
    out_cfg->clint_base = 0x02000000ULL;
    out_cfg->dram_base = 0x80000000ULL;
    out_cfg->dram_size = 0x08000000ULL; /* 128 MiB baseline */
    return 0;
}

int hal_riscv_bsp_init(const riscv_bsp_config_t* cfg) {
    if (!cfg) {
        return -1;
    }

    if (cfg->uart_base == 0ULL || cfg->plic_base == 0ULL || cfg->clint_base == 0ULL) {
        return -1;
    }

    g_active_cfg = *cfg;
    return 0;
}

const riscv_bsp_config_t* hal_riscv_bsp_active(void) {
    return &g_active_cfg;
}
