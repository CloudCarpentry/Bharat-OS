#ifndef BHARAT_BOOT_UBOOT_ADAPTER_H
#define BHARAT_BOOT_UBOOT_ADAPTER_H

#include <boot/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse U-Boot FDT handoff into canonical boot_info_t
int uboot_adapter_parse(const void *fdt_blob, boot_info_t *out_bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_UBOOT_ADAPTER_H
