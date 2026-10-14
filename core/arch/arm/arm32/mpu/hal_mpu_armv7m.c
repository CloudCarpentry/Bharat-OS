#include "hal/hal_mpu.h"
#include "hal/hal_pt.h"
#include "kernel/status.h"
#include "console/console_core.h"
#include <stdint.h>
#include <stddef.h>

#define MPU_TYPE (*(volatile uint32_t *)0xE000ED90)
#define MPU_CTRL (*(volatile uint32_t *)0xE000ED94)
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9C)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0)

#define MPU_CTRL_ENABLE    (1 << 0)
#define MPU_CTRL_HFNMIENA  (1 << 1)
#define MPU_CTRL_PRIVDEFENA (1 << 2)

#define MPU_RASR_ENABLE    (1 << 0)

static hal_mpu_caps_t armv7m_mpu_caps = {0};

static int armv7m_mpu_program_region(uint32_t region_id, phys_addr_t base, size_t size, uint32_t flags) {
    if (region_id >= armv7m_mpu_caps.max_regions) {
        return K_ERR_INVALID_ARG;
    }

    // Constraints check
    if (size < armv7m_mpu_caps.min_region_alignment) {
        return K_ERR_INVALID_ARG;
    }

    if ((size & (size - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }

    if ((base % size) != 0) {
        return K_ERR_INVALID_ARG;
    }

    // Calculate Size field (SIZE = log2(size) - 1)
    uint32_t size_field = 0;
    for (int i = 0; i < 32; i++) {
        if ((1ULL << i) == size) {
            size_field = i - 1;
            break;
        }
    }

    // Map prot flags to AP bits
    // By default, if HAL_PT_FLAG_USER is not set, it's privileged access only.
    // If HAL_PT_FLAG_READ is set without WRITE, it's RO.
    uint32_t ap = 0;
    if (flags & HAL_PT_FLAG_USER) {
        if (flags & HAL_PT_FLAG_WRITE) {
            ap = 0b011; // Full access
        } else {
            ap = 0b010; // Privileged RO, Unprivileged RO
        }
    } else {
        if (flags & HAL_PT_FLAG_WRITE) {
            ap = 0b001; // Privileged RW, Unprivileged No Access
        } else {
            ap = 0b101; // Privileged RO, Unprivileged No Access
        }
    }

    uint32_t xn = (flags & HAL_PT_FLAG_EXEC) ? 0 : 1;

    // Normal memory by default, C=1, B=1, S=1 for simplicity
    uint32_t rasr = (xn << 28) | (ap << 24) | (0b000 << 19) | (1 << 18) | (1 << 17) | (1 << 16) | (0x00 << 8) | (size_field << 1) | MPU_RASR_ENABLE;

    __asm__ volatile("dmb" ::: "memory");

    MPU_RNR = region_id;
    MPU_RBAR = (uint32_t)base;
    MPU_RASR = rasr;

    __asm__ volatile("dsb\n\tisb" ::: "memory");

    return K_OK;
}

static int armv7m_mpu_disable_region(uint32_t region_id) {
    if (region_id >= armv7m_mpu_caps.max_regions) {
        return K_ERR_INVALID_ARG;
    }

    __asm__ volatile("dmb" ::: "memory");

    MPU_RNR = region_id;
    MPU_RASR &= ~MPU_RASR_ENABLE;

    __asm__ volatile("dsb\n\tisb" ::: "memory");

    return K_OK;
}

static const hal_mpu_caps_t* armv7m_mpu_get_caps(void) {
    return &armv7m_mpu_caps;
}

static hal_mpu_ops_t armv7m_mpu_ops = {
    .program_region = armv7m_mpu_program_region,
    .disable_region = armv7m_mpu_disable_region,
    .get_caps = armv7m_mpu_get_caps
};

void armv7m_mpu_register_backend(void) {
    // Only register on genuine Cortex-M hardware.
    // The actual Cortex-M board bring-up must call this registration after
    // verifying compatible hardware and before untrusted application execution.
    // We don't blindly program MPU_CTRL or register it if not called from the board.

    uint32_t type = MPU_TYPE;
    uint32_t dregions = (type >> 8) & 0xFF;

    if (dregions == 0) {
        return; // No MPU
    }

    armv7m_mpu_caps.max_regions = dregions;
    armv7m_mpu_caps.supports_subregions = true;
    armv7m_mpu_caps.requires_power_of_two_size = true;
    armv7m_mpu_caps.min_region_alignment = 32;

    hal_mpu_register_ops(&armv7m_mpu_ops);

    __asm__ volatile("dmb" ::: "memory");

    // Enable MPU with default memory map for privileged access
    // and enable Memory Management Faults.
    MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;

    __asm__ volatile("dsb\n\tisb" ::: "memory");
}
