#include "boot/adapters/fdt_adapter.h"
#include "boot/boot_errno.h"

int fdt_adapter_parse(const void *fdt_blob, boot_info_t *out_bi) {
    if (!fdt_blob || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    boot_info_init(out_bi);

    // Default architecture guess from FDT, or leave generic
    out_bi->source = BOOT_SOURCE_UNKNOWN; // Caller (OpenSBI, U-Boot) might specify this

    out_bi->firmware.fdt_ptr = (void *)fdt_blob;

    // Abstracted FDT walking
    // 1. check magic `0xd00dfeed`
    // 2. find /chosen -> bootargs -> boot_info_set_cmdline()
    // 3. find /memory -> boot_info_add_mem_region()
    // 4. find /chosen -> linux,initrd-start/end -> boot_info_add_module()

    return BOOT_OK;
}
