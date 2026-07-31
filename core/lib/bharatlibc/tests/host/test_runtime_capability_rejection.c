#include <bharat/bsys/runtime_caps.h>
#include <standard/assert.h>
#include <standard/stddef.h>

int main(void) {
    bh_bsys_runtime_caps_t caps;
    caps.abi_version = 1;
    caps.structure_size = sizeof(caps);
    caps.feature_bits[0] = 0;

    /* 1. Incompatible pointer width */
#if defined(__LP64__) || defined(_LP64) || defined(__x86_64__) || defined(__aarch64__) || defined(__riscv) && (__riscv_xlen == 64)
    caps.pointer_width = 32; /* Incompatible! */
#else
    caps.pointer_width = 64; /* Incompatible! */
#endif

    int rc = bh_bsys_validate_capabilities(&caps);
    assert(rc != 0); /* Must reject incompatible pointer widths */

    /* 2. Incompatible ABI version */
    caps.abi_version = 0; /* Too old */
    rc = bh_bsys_validate_capabilities(&caps);
    assert(rc != 0); /* Must reject */

    return 0;
}
