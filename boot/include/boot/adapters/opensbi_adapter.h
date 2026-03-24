#ifndef BHARAT_BOOT_OPENSBI_ADAPTER_H
#define BHARAT_BOOT_OPENSBI_ADAPTER_H

#include <boot/boot_info.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Parse OpenSBI entry handoff (hartid, fdt) into canonical boot_info_t
int opensbi_adapter_parse(uint64_t hart_id, const void *fdt_blob, boot_info_t *out_bi);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_BOOT_OPENSBI_ADAPTER_H
