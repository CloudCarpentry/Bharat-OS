#include "arch/arch_cpu_caps.h"
#include "../../lib/msg/crc.h"

// Forward declare architecture specific backend registrations if they exist
#if defined(__aarch64__)
extern uint32_t bharat_msg_crc32_aarch64(const uint8_t *data, size_t len);
#endif

void bharat_algorithm_backends_init(void) {
#if defined(__aarch64__)
    if (arch_cpu_has_system_all(ARCH_CPU_FEAT_ARM64_CRC32)) {
        bharat_msg_crc_register_backend(bharat_msg_crc32_aarch64);
    }
#endif
}
