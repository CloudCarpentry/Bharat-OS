#ifndef BHARAT_KERNEL_H
#define BHARAT_KERNEL_H

/* Bharat-OS Kernel Primary Header */

#include "mm.h"
#include "boot/boot_info.h"
void kernel_main_common(const boot_info_t *boot);

void kernel_panic(const char *message);

#endif // BHARAT_KERNEL_H
