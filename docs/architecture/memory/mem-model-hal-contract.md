# Memory Model HAL Contract

## Overview
Standardizes how the kernel validates hardware memory capabilities against the selected system profile.

## HAL Capabilities
- `supports_mmu_full`
- `supports_mmu_lite`
- `supports_mpu_only`
- `supports_user_kernel_split`
- `supports_page_protection`
- `supports_execute_disable`

## Boot Validation
The kernel calls `mem_model_validate_hal_caps` early in the boot sequence. If the hardware does not support the required features of the selected memory model (e.g., MMU_FULL on an MPU system), the kernel fails closed with a panic.
