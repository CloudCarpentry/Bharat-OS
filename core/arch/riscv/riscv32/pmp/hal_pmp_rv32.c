#include "hal/hal_mpu.h"
#include "hal/hal_pt.h"
#include "kernel/status.h"
#include "console/console_core.h"
#include "pmp/hal_pmp_rv32.h"
#include <stdint.h>
#include <stddef.h>

#define PMP_MAX_REGIONS 16

static hal_mpu_caps_t rv32_pmp_caps = {0};

static inline void write_pmpcfg0(uint32_t val) {
    __asm__ volatile("csrw 0x3A0, %0" :: "rK"(val) : "memory");
}

static inline uint32_t read_pmpcfg0(void) {
    uint32_t val;
    __asm__ volatile("csrr %0, 0x3A0" : "=r"(val) :: "memory");
    return val;
}

static inline void write_pmpaddr(int index, uint32_t val) {
    // Only handling basic writes using macros might require a switch depending on CSRs.
    // However, PMP registers are often accessed via dynamic CSR writes (if supported by toolchain/assembler macros)
    // Or we must do it manually for up to PMP_MAX_REGIONS.
    // For simplicity we will handle up to 16.
    switch (index) {
        case 0:  __asm__ volatile("csrw 0x3B0, %0" :: "r"(val) : "memory"); break;
        case 1:  __asm__ volatile("csrw 0x3B1, %0" :: "r"(val) : "memory"); break;
        case 2:  __asm__ volatile("csrw 0x3B2, %0" :: "r"(val) : "memory"); break;
        case 3:  __asm__ volatile("csrw 0x3B3, %0" :: "r"(val) : "memory"); break;
        case 4:  __asm__ volatile("csrw 0x3B4, %0" :: "r"(val) : "memory"); break;
        case 5:  __asm__ volatile("csrw 0x3B5, %0" :: "r"(val) : "memory"); break;
        case 6:  __asm__ volatile("csrw 0x3B6, %0" :: "r"(val) : "memory"); break;
        case 7:  __asm__ volatile("csrw 0x3B7, %0" :: "r"(val) : "memory"); break;
        case 8:  __asm__ volatile("csrw 0x3B8, %0" :: "r"(val) : "memory"); break;
        case 9:  __asm__ volatile("csrw 0x3B9, %0" :: "r"(val) : "memory"); break;
        case 10: __asm__ volatile("csrw 0x3BA, %0" :: "r"(val) : "memory"); break;
        case 11: __asm__ volatile("csrw 0x3BB, %0" :: "r"(val) : "memory"); break;
        case 12: __asm__ volatile("csrw 0x3BC, %0" :: "r"(val) : "memory"); break;
        case 13: __asm__ volatile("csrw 0x3BD, %0" :: "r"(val) : "memory"); break;
        case 14: __asm__ volatile("csrw 0x3BE, %0" :: "r"(val) : "memory"); break;
        case 15: __asm__ volatile("csrw 0x3BF, %0" :: "r"(val) : "memory"); break;
        default: break;
    }
}

static inline uint32_t read_pmpcfg(int index) {
    uint32_t val = 0;
    switch (index) {
        case 0: __asm__ volatile("csrr %0, 0x3A0" : "=r"(val)); break;
        case 1: __asm__ volatile("csrr %0, 0x3A1" : "=r"(val)); break;
        case 2: __asm__ volatile("csrr %0, 0x3A2" : "=r"(val)); break;
        case 3: __asm__ volatile("csrr %0, 0x3A3" : "=r"(val)); break;
    }
    return val;
}

static inline void write_pmpcfg(int index, uint32_t val) {
    switch (index) {
        case 0: __asm__ volatile("csrw 0x3A0, %0" :: "r"(val) : "memory"); break;
        case 1: __asm__ volatile("csrw 0x3A1, %0" :: "r"(val) : "memory"); break;
        case 2: __asm__ volatile("csrw 0x3A2, %0" :: "r"(val) : "memory"); break;
        case 3: __asm__ volatile("csrw 0x3A3, %0" :: "r"(val) : "memory"); break;
    }
}

static int rv32_pmp_program_region(uint32_t region_id, phys_addr_t base, size_t size, uint32_t flags) {
    if (region_id >= rv32_pmp_caps.max_regions) {
        return K_ERR_INVALID_ARG;
    }

    if (size < rv32_pmp_caps.min_region_alignment) {
        return K_ERR_INVALID_ARG;
    }

    if ((size & (size - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }

    if ((base % size) != 0) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t cfg_idx = region_id / 4;
    uint32_t shift = (region_id % 4) * 8;

    uint32_t pmp_flags = 0;
    if (flags & HAL_PT_FLAG_READ) pmp_flags |= RV32_PMP_R;
    if (flags & HAL_PT_FLAG_WRITE) pmp_flags |= RV32_PMP_W;
    if (flags & HAL_PT_FLAG_EXEC) pmp_flags |= RV32_PMP_X;

    // Determine PMP Address Mode
    uint32_t mode = RV32_PMP_A_NAPOT;
    if (size == 4) {
        mode = RV32_PMP_A_NA4;
    }

    uint32_t cfg_val = read_pmpcfg(cfg_idx);

    // Check if locked
    if ((cfg_val >> shift) & RV32_PMP_L) {
        return K_ERR_DENIED;
    }

    // Do NOT set L bit during normal domain reprogramming as this would prevent later updates.
    cfg_val &= ~(0xFF << shift);
    cfg_val |= ((pmp_flags | mode) << shift);

    uint32_t addr_val;
    if (mode == RV32_PMP_A_NAPOT) {
        addr_val = (base >> 2) | ((size >> 3) - 1);
    } else {
        addr_val = (base >> 2);
    }

    write_pmpaddr(region_id, addr_val);
    write_pmpcfg(cfg_idx, cfg_val);

    return K_OK;
}

static int rv32_pmp_disable_region(uint32_t region_id) {
    if (region_id >= rv32_pmp_caps.max_regions) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t cfg_idx = region_id / 4;
    uint32_t shift = (region_id % 4) * 8;

    uint32_t cfg_val = read_pmpcfg(cfg_idx);

    // Check if locked
    if ((cfg_val >> shift) & RV32_PMP_L) {
        return K_ERR_DENIED;
    }

    cfg_val &= ~(0xFF << shift);
    cfg_val |= (RV32_PMP_A_OFF << shift);

    write_pmpcfg(cfg_idx, cfg_val);

    return K_OK;
}

static const hal_mpu_caps_t* rv32_pmp_get_caps(void) {
    return &rv32_pmp_caps;
}

static hal_mpu_ops_t rv32_pmp_ops = {
    .program_region = rv32_pmp_program_region,
    .disable_region = rv32_pmp_disable_region,
    .get_caps = rv32_pmp_get_caps
};

void rv32_pmp_register_backend(void) {
#if defined(BHARAT_RISCV_PRIV_MODE_M) && defined(BHARAT_KERNEL_OWNS_PMP)
    // Target explicitly guarantees we are in bare-metal M-mode
    // and the kernel is permitted to own PMP CSRs.

    // Check if PMP is actually implemented by writing and reading back
    // (a real implementation would trap or do this carefully, but here
    // we assume the target board wouldn't set the above macros if not present).

    rv32_pmp_caps.max_regions = PMP_MAX_REGIONS;
    rv32_pmp_caps.supports_subregions = false;
    rv32_pmp_caps.requires_power_of_two_size = true;
    rv32_pmp_caps.min_region_alignment = 4;

    hal_mpu_register_ops(&rv32_pmp_ops);
#else
    // Target does not guarantee M-mode or PMP ownership.
    // Do not register PMP backend to prevent illegal instruction traps in S-mode.
    return;
#endif
}
