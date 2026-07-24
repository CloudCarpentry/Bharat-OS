#include <bharat/uapi/init/init_boot_context.h>
#include <stddef.h>

bool init_boot_context_is_valid(const init_boot_context_t *ctx) {
    if (ctx == NULL) {
        return false;
    }

    // Reject invalid ABI version
    if (ctx->abi_version != BHARAT_INIT_BOOT_CONTEXT_ABI_VERSION) {
        return false;
    }

    // Personality ID must be within valid range (0-31 as per Task 4)
    if (ctx->personality_id > 31) {
        return false;
    }

    // Kernel health level must be valid
    if (ctx->kernel_health.level > INIT_KERNEL_HEALTH_UNSAFE) {
        return false;
    }

    return true;
}
