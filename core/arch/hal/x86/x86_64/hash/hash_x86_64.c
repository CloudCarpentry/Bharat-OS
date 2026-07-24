/*
 * hal/x86_64/hash/hash_x86_64.c
 *
 * HAL layer must stay policy/abstraction-only. ISA-optimized hash logic now lives
 * under core/arch/x86/x86_64/hash/.
 */

#include "arch/arch_cpu_caps.h"
#include <stdbool.h>

bool hal_hash_x86_64_runtime_accel_available(void) {
    return arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_AES) ||
           arch_cpu_has_system_any(ARCH_CPU_FEAT_X86_PCLMUL);
}
