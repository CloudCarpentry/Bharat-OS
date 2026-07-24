#include "boot/boot_info.h"

// Basic memcmp/memcpy for freestanding env if needed
static size_t my_strlen(const char *str) {
    size_t len = 0;
    while (str && str[len] != '\0') {
        len++;
    }
    return len;
}

void boot_info_init(boot_info_t *bi) {
    if (!bi) return;

    // Zero out the struct
    char *p = (char *)bi;
    for (size_t i = 0; i < sizeof(boot_info_t); i++) {
        p[i] = 0;
    }

    bi->magic = BHARAT_BOOT_INFO_MAGIC;
    bi->version = 1;

    bi->source = BOOT_SOURCE_UNKNOWN;
    bi->arch = BOOT_ARCH_UNKNOWN;

    bi->selected_mode = BOOT_MODE_AUTOMOTIVE;
    bi->security_state = BOOT_SECURITY_UNKNOWN;

    bi->is_degraded = false;
    bi->degraded_reasons_mask = 0;

    bi->is_validated = false;
}

int boot_info_add_mem_region(boot_info_t *bi, uint64_t phys_start, uint64_t size, boot_mem_type_t type) {
    if (!bi || bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        return -1; // BOOT_ERR_BAD_MAGIC
    }

    if (size == 0) {
        return -2; // BOOT_ERR_INVALID_MEM_RANGE
    }

    if (bi->mem_region_count >= BHARAT_BOOT_MAX_MEM_REGIONS) {
        return -3; // BOOT_ERR_TOO_MANY_MEM_REGIONS
    }

    bi->mem_regions[bi->mem_region_count].phys_start = phys_start;
    bi->mem_regions[bi->mem_region_count].size = size;
    bi->mem_regions[bi->mem_region_count].type = type;

    bi->mem_region_count++;

    return 0; // BOOT_OK
}

int boot_info_add_module(boot_info_t *bi, uint64_t phys_start, uint64_t size, const char *name) {
    if (!bi || bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        return -1;
    }

    if (size == 0) {
        return -2;
    }

    if (bi->module_count >= BHARAT_BOOT_MAX_MODULES) {
        return -4; // BOOT_ERR_TOO_MANY_MODULES
    }

    bi->modules[bi->module_count].phys_start = phys_start;
    bi->modules[bi->module_count].size = size;
    bi->modules[bi->module_count].name = name;

    bi->module_count++;

    return 0;
}

int boot_info_set_cmdline(boot_info_t *bi, const char *cmdline, size_t len) {
    if (!bi || bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        return -1;
    }

    if (!cmdline) return 0;

    size_t actual_len = len;
    if (actual_len == 0) {
        actual_len = my_strlen(cmdline);
    }

    if (actual_len >= BHARAT_BOOT_CMDLINE_MAX_LEN) {
        actual_len = BHARAT_BOOT_CMDLINE_MAX_LEN - 1;
    }

    for (size_t i = 0; i < actual_len; i++) {
        bi->cmdline[i] = cmdline[i];
    }
    bi->cmdline[actual_len] = '\0';

    return 0;
}

int boot_info_finalize(boot_info_t *bi) {
    if (!bi || bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        return -1;
    }

    // A real finalize might do things like map overlapping regions,
    // or sort the memory map.
    // We leave the logic minimal here as validation phase will check the result.
    return 0;
}
