#ifndef BHARAT_BOOT_FDT_ADAPTER_H
#define BHARAT_BOOT_FDT_ADAPTER_H

#include <boot/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse flattened device tree into canonical boot_info_t
// Works for ARM64 and generic RISC-V handoffs
int fdt_adapter_parse(const void *fdt_blob, boot_info_t *out_bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_FDT_ADAPTER_H
