#include "boot/adapters/uefi_adapter.h"
#include "boot/boot_errno.h"

int uefi_adapter_parse(const void *efi_system_table, boot_info_t *out_bi) {
    if (!efi_system_table || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    boot_info_init(out_bi);
    out_bi->source = BOOT_SOURCE_UEFI;

    // Placeholder: We are explicitly not building a full UEFI loader in this task.
    // We are returning UNSUPPORTED because the user asked for this to be a "compile-safe placeholder"
    // that returns an error unless fully enabled.

    return BOOT_ERR_UNSUPPORTED;
}
