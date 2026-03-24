#ifndef BHARAT_BOOT_MULTIBOOT2_ADAPTER_H
#define BHARAT_BOOT_MULTIBOOT2_ADAPTER_H

#include <boot/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parses raw multiboot2 structures and populates canonical boot_info_t
int multiboot2_adapter_parse(const void *mb2_info, boot_info_t *out_bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_MULTIBOOT2_ADAPTER_H
