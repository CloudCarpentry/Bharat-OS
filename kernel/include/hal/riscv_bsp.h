#ifndef BHARAT_HAL_RISCV_BSP_H
#define BHARAT_HAL_RISCV_BSP_H

#include <stdint.h>

typedef enum {
    RISCV_SOC_QEMU_VIRT = 0,
    RISCV_SOC_SHAKTI_E,
    RISCV_SOC_SHAKTI_C,
    RISCV_SOC_SHAKTI_I,
} riscv_soc_profile_t;

typedef struct {
    riscv_soc_profile_t soc_profile;
    uint64_t fdt_ptr;
    uint64_t uart_base;
    uint64_t plic_base;
    uint64_t clint_base;
    uint64_t dram_base;
    uint64_t dram_size;
} riscv_bsp_config_t;

int hal_riscv_bsp_detect(const char* profile, uint64_t fdt_ptr, riscv_bsp_config_t* out_cfg);
int hal_riscv_bsp_init(const riscv_bsp_config_t* cfg);
const riscv_bsp_config_t* hal_riscv_bsp_active(void);

void hal_riscv_set_boot_info(uint64_t hart_id, uint64_t fdt_ptr);
uint64_t hal_riscv_boot_hart_id(void);
uint64_t hal_riscv_boot_fdt_ptr(void);

#endif // BHARAT_HAL_RISCV_BSP_H
