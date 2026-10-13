#include "boot/bh_boot_context.h"
#include <string.h>

void bh_boot_context_init(struct bh_boot_context *ctx) {
    if (!ctx) return;

    // Clear the context
    for (uint32_t i = 0; i < sizeof(struct bh_boot_context); i++) {
        ((uint8_t*)ctx)[i] = 0;
    }

    ctx->version = 1;
    ctx->size = sizeof(struct bh_boot_context);
    ctx->arch = BH_ARCH_ID_UNKNOWN;
    ctx->platform = BH_PLATFORM_ID_UNKNOWN;
    ctx->profile = BH_PROFILE_ID_UNKNOWN;
}
