#include "hal/riscv_bsp.h"
#include "hal/hal_boot.h"
#include "../common/fdt_parser.h"

#include <stddef.h>
#include <stdint.h>

static riscv_bsp_config_t g_active_cfg;

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

int hal_riscv_bsp_detect(const char* profile, uint64_t fdt_ptr, riscv_bsp_config_t* out_cfg) {
    if (!profile || !out_cfg) {
        return -1;
    }

    cfg_clear(out_cfg);
    out_cfg->fdt_ptr = fdt_ptr;

    // Use common fdt_parser
    if (fdt_ptr != 0ULL) {
        bharat_boot_info_t boot_info;
        fdt_devices_t devices;

        if (fdt_parse((const void*)fdt_ptr, &boot_info, &devices) == 0) {
            if (devices.uart_base != 0) out_cfg->uart_base = devices.uart_base;
            if (devices.plic_base != 0) out_cfg->plic_base = devices.plic_base;
            if (devices.clint_base != 0) out_cfg->clint_base = devices.clint_base;

            if (boot_info.mem_region_count > 0) {
                out_cfg->dram_base = boot_info.mem_regions[0].base;
                out_cfg->dram_size = boot_info.mem_regions[0].size;
            }

            // Optional: determine specific SHAKTI board based on FDT compatible string if needed
            // Currently relies on the passed-in profile parameter
        }
    }

    // Static Fallbacks for explicit profiles
    if (str_eq(profile, "shakti-e")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_E;
        if (!out_cfg->uart_base) out_cfg->uart_base = 0xF0000000ULL;
        if (!out_cfg->plic_base) out_cfg->plic_base = 0x0C000000ULL;
        if (!out_cfg->clint_base) out_cfg->clint_base = 0x02000000ULL;
        if (!out_cfg->dram_base) out_cfg->dram_base = 0x80000000ULL;
        if (!out_cfg->dram_size) out_cfg->dram_size = 0x10000000ULL;
        return 0;
    }

    if (str_eq(profile, "shakti-c")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_C;
        if (!out_cfg->uart_base) out_cfg->uart_base = 0xE0001800ULL;
        if (!out_cfg->plic_base) out_cfg->plic_base = 0x0C000000ULL;
        if (!out_cfg->clint_base) out_cfg->clint_base = 0x02000000ULL;
        if (!out_cfg->dram_base) out_cfg->dram_base = 0x80000000ULL;
        if (!out_cfg->dram_size) out_cfg->dram_size = 0x40000000ULL;
        return 0;
    }

    if (str_eq(profile, "shakti-i")) {
        out_cfg->soc_profile = RISCV_SOC_SHAKTI_I;
        if (!out_cfg->uart_base) out_cfg->uart_base = 0x20000000ULL;
        if (!out_cfg->plic_base) out_cfg->plic_base = 0x0C000000ULL;
        if (!out_cfg->clint_base) out_cfg->clint_base = 0x02000000ULL;
        if (!out_cfg->dram_base) out_cfg->dram_base = 0x80000000ULL;
        if (!out_cfg->dram_size) out_cfg->dram_size = 0x20000000ULL;
        return 0;
    }

    // QEMU fallback
    out_cfg->soc_profile = RISCV_SOC_QEMU_VIRT;
    if (!out_cfg->uart_base) out_cfg->uart_base = 0x10000000ULL;
    if (!out_cfg->plic_base) out_cfg->plic_base = 0x0C000000ULL;
    if (!out_cfg->clint_base) out_cfg->clint_base = 0x02000000ULL;
    if (!out_cfg->dram_base) out_cfg->dram_base = 0x80000000ULL;
    if (!out_cfg->dram_size) out_cfg->dram_size = 0x08000000ULL;

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
