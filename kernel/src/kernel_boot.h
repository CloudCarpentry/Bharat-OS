#ifndef BHARAT_KERNEL_BOOT_H
#define BHARAT_KERNEL_BOOT_H

#include "boot/boot_info.h"

void boot_common_early(const boot_info_t *boot);
void boot_common_security(const boot_info_t *boot);
void boot_common_memory(const boot_info_t *boot);
void boot_common_platform_services(const boot_info_t *boot);
void boot_common_runtime(const boot_info_t *boot);

#endif // BHARAT_KERNEL_BOOT_H
