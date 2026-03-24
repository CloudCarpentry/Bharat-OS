#include "boot/adapters/uboot_adapter.h"
#include "boot/adapters/fdt_adapter.h"
#include "boot/boot_errno.h"

int uboot_adapter_parse(const void *fdt_blob, boot_info_t *out_bi) {
    if (!fdt_blob || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    // Use common FDT logic
    int ret = fdt_adapter_parse(fdt_blob, out_bi);
    if (ret != BOOT_OK) {
        return ret;
    }

    // U-Boot specific flags or overrides
    out_bi->source = BOOT_SOURCE_UBOOT_FDT;

    // U-Boot FDT might be passing specific secure/measured boot nodes
    // which could be populated here.
    out_bi->security_state = BOOT_SECURITY_UNKNOWN; // By default without reading FIT metadata

    return BOOT_OK;
}
