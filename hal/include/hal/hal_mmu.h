#pragma once

#include <stdbool.h>

enum hal_memory_model {
    HAL_MEMORY_MODEL_NONE = 0,
    HAL_MEMORY_MODEL_MPU,
    HAL_MEMORY_MODEL_MMU_LITE,
    HAL_MEMORY_MODEL_MMU_FULL,
};

bool hal_memory_model_supported(enum hal_memory_model model);
