#include <stdint.h>
#include <stddef.h>
#include "sched/sched.h"
#include "bharat/mem_class.h"
#include "mm/mem_model.h"
#include "mm/mem_validator.h"
#include "kernel/status.h"

static uint64_t class_allocations[MEM_CLASS_MAX] = {0};

/**
 * Validates whether a memory class is supported by the current memory model.
 * Tier U (Universal) classes are supported everywhere.
 * Tier P (Profile-conditional) classes require at least MMU_LITE.
 */
bool mem_class_is_supported(alloc_class_t cls, mem_model_t model) {
    if (cls < MEM_TENSOR) {
        return true; // Legacy/Basic classes
    }

    /* Tier U: Universal AI-adjacent primitives */
    if (cls == MEM_TENSOR || cls == MEM_MODEL_RO || cls == MEM_SCRATCH_LOWLAT) {
        return true;
    }

    /* Tier P: Profile-conditional AI-adjacent primitives */
    if (cls == MEM_TENSOR_PINNED || cls == MEM_STREAM_DMA ||
        cls == MEM_SECURE_MODEL || cls == MEM_SHARED_ACCEL) {

        // MPU-only targets must reject Tier P classes
        if (model == MEM_MODEL_MPU) {
            return false;
        }

        return true;
    }

    return false;
}

int sys_mem_alloc_class(size_t size, uint32_t mem_class, uint32_t flags, uint64_t* out_addr) {
    if (!out_addr) return K_ERR_INVALID_ARG;

    mem_model_t model = mm_get_validated_model();
    if (!mem_class_is_supported((alloc_class_t)mem_class, model)) {
        return K_ERR_PROFILE_RESTRICTED;
    }

    if (mem_class < MEM_CLASS_MAX) {
        __atomic_fetch_add(&class_allocations[mem_class], size, __ATOMIC_RELAXED);
    } else {
        return K_ERR_INVALID_ARG;
    }

    // In a real implementation this would map memory.
    // For V1/stub we return a simulated handle.
    uint64_t handle = 0xDEADBEEF0000 | (uint64_t)mem_class;
    *out_addr = handle;

    return K_OK;
}
