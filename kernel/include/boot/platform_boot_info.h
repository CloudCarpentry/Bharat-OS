#ifndef BHARAT_BOOT_PLATFORM_BOOT_INFO_H
#define BHARAT_BOOT_PLATFORM_BOOT_INFO_H

#include <stdint.h>
#include <stdbool.h>
#include "bharat/display/boot_video.h"

typedef enum {
    BHARAT_EARLY_UART_TYPE_NONE = 0,
    BHARAT_EARLY_UART_TYPE_PL011 = 1,
    BHARAT_EARLY_UART_TYPE_NS16550 = 2,
    BHARAT_EARLY_UART_TYPE_SBI = 3,
} bharat_early_uart_type_t;

// Platform-resolved early boot metadata
// Derived from raw boot info, used to initialize normalized boot info
typedef struct platform_boot_info {
    // Early UART details
    bool has_early_console;
    uint64_t uart_phys_base;
    uint32_t uart_type;      // bharat_early_uart_type_t
    uint32_t uart_clock;     // Hz
    uint32_t uart_baudrate;  // bps

    // Machine memory quirks
    uint64_t ram_base;
    uint64_t ram_size_mb;

    // Optional board-specific reserved ranges
    uint64_t board_reserved_base;
    uint64_t board_reserved_size;

    // Discovered video handoff (from SimpleFB, EFI, Multiboot)
    boot_video_handoff_t video;

    // Timer/Interrupt hints
    uint64_t timer_freq;
    uint64_t gic_dist_base;
    uint64_t gic_redist_base;
    uint64_t gic_cpu_base;
    uint64_t plic_base;
    uint64_t clint_base;

    // Security/provenance
    bool secure_boot_passed;

} platform_boot_info_t;

#endif // BHARAT_BOOT_PLATFORM_BOOT_INFO_H
