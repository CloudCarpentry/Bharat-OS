#ifndef BHARAT_KERNEL_H
#define BHARAT_KERNEL_H

/* Bharat-OS Kernel Primary Header */

#include "mm.h"
#if defined(__x86_64__)
#include "../src/boot/x86_64/multiboot2.h"
void kernel_main(uint32_t magic, multiboot_information_t* mb_info);
#elif defined(__riscv)
#include <stdint.h>
void kernel_main(uint64_t hart_id, uintptr_t fdt_ptr);
#else
void kernel_main(void);
#endif

void kernel_panic(const char *message);

#endif // BHARAT_KERNEL_H
