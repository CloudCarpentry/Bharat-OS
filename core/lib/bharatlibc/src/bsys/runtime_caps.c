#include <bharat/bsys/runtime_caps.h>
#include <bharat/libc/config.h>

int bh_bsys_validate_capabilities(const bh_bsys_runtime_caps_t *caps) {
    if (!caps) return -1;

    /* 1. ABI version check */
    if (caps->abi_version < BH_LIBC_ABI_VERSION) {
        return -2; /* Runtime has too old ABI version */
    }

    /* 2. Pointer width and architecture checks */
#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__) || defined(__riscv) && (__riscv_xlen == 64)
    if (caps->pointer_width != 64) {
        return -3; /* Expected 64-bit runtime */
    }
#else
    if (caps->pointer_width != 32) {
        return -4; /* Expected 32-bit runtime */
    }
#endif

    /* 3. Memory model mismatch check */
#if defined(BH_LIBC_MEMORY_MPU)
    if (caps->memory_model != BH_CAP_MEM_MPU) {
        return -5; /* MPU library expects MPU runtime */
    }
#elif defined(BH_LIBC_MEMORY_MMU_LITE)
    if (caps->memory_model != BH_CAP_MEM_MMU_LITE && caps->memory_model != BH_CAP_MEM_MMU_FULL) {
        return -6; /* MMU Lite library expects MMU runtime */
    }
#elif defined(BH_LIBC_MEMORY_MMU_FULL)
    if (caps->memory_model != BH_CAP_MEM_MMU_FULL) {
        return -7; /* MMU Full library expects MMU Full runtime */
    }
#endif

    /* 4. Feature checks */
#if defined(BH_LIBC_PROFILE_RT_SAFE)
    if (caps->feature_bits[0] & BH_FEATURE_ALLOC_UNBOUNDED) {
        return -8; /* RT profile forbids unbounded allocator */
    }
#endif

    return 0; /* Compatible */
}
