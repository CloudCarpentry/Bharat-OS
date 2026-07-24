#ifndef BHARAT_BOOT_UEFI_ADAPTER_H
#define BHARAT_BOOT_UEFI_ADAPTER_H

#include <boot/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Compile-safe placeholder for UEFI loader handoff
int uefi_adapter_parse(const void *efi_system_table, boot_info_t *out_bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_UEFI_ADAPTER_H
