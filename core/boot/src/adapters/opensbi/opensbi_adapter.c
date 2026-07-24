#include "boot/adapters/opensbi_adapter.h"
#include "boot/adapters/fdt_adapter.h"
#include "boot/boot_errno.h"

int opensbi_adapter_parse(uint64_t hart_id, const void *fdt_blob, boot_info_t *out_bi) {
    if (!fdt_blob || !out_bi) return BOOT_ERR_INVALID_ARGUMENT;

    // OpenSBI relies completely on FDT for memory/cmdline metadata
    int ret = fdt_adapter_parse(fdt_blob, out_bi);
    if (ret != BOOT_OK) {
        return ret;
    }

    out_bi->source = BOOT_SOURCE_OPENSBI_FDT;
    out_bi->arch = BOOT_ARCH_RISCV64; // Might be RV32 based on build
    out_bi->boot_cpu_id = hart_id;

    return BOOT_OK;
}
