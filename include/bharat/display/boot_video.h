#ifndef BHARAT_BOOT_VIDEO_H
#define BHARAT_BOOT_VIDEO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bharat/display/pixel_format.h"

typedef enum {
    BOOT_VIDEO_PATH_NONE = 0,
    BOOT_VIDEO_PATH_TEXT_ONLY,
    BOOT_VIDEO_PATH_FIRMWARE_FB,
    BOOT_VIDEO_PATH_SIMPLE_CONTROLLER,
    BOOT_VIDEO_PATH_VIRTIO_GPU_LATE,
    BOOT_VIDEO_PATH_FULL_GPU_LATE
} boot_video_path_t;

typedef struct boot_video_handoff {
    bool valid;
    boot_video_path_t path;

    uintptr_t phys_addr;
    uintptr_t virt_addr;
    size_t size;

    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    pixel_format_t format;

    uint32_t flags;  // write-combine, cache-flush-needed, firmware-owned, etc.
    uint32_t source; // UEFI_GOP, DT_SIMPLEFB, ACPI, BOARD_EARLY_LCD, etc.
} boot_video_handoff_t;

// Standard source flags
#define BOOT_VIDEO_SOURCE_UNKNOWN      0
#define BOOT_VIDEO_SOURCE_UEFI_GOP     1
#define BOOT_VIDEO_SOURCE_DT_SIMPLEFB  2
#define BOOT_VIDEO_SOURCE_ACPI         3
#define BOOT_VIDEO_SOURCE_BOARD_LCD    4

#endif // BHARAT_BOOT_VIDEO_H
