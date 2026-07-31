#include <bharat/bsys/runtime_caps.h>
#include <standard/assert.h>
#include <standard/stddef.h>

int main(void) {
    bh_bsys_runtime_caps_t caps;
    caps.abi_version = 1;
    caps.structure_size = sizeof(caps);
    caps.feature_bits[0] = 0; /* No prohibited features */
#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__) || defined(__riscv) && (__riscv_xlen == 64)
    caps.pointer_width = 64;
#else
    caps.pointer_width = 32;
#endif

#if defined(BH_LIBC_MEMORY_MPU)
    caps.memory_model = BH_CAP_MEM_MPU;
#elif defined(BH_LIBC_MEMORY_MMU_LITE)
    caps.memory_model = BH_CAP_MEM_MMU_LITE;
#elif defined(BH_LIBC_MEMORY_MMU_FULL)
    caps.memory_model = BH_CAP_MEM_MMU_FULL;
#else
    caps.memory_model = BH_CAP_MEM_MMU_FULL; /* Default compatible with HOST/general */
#endif

    int rc = bh_bsys_validate_capabilities(&caps);
    assert(rc == 0); /* Should pass validation */

    return 0;
}
