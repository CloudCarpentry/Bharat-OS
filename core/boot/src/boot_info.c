#include "boot/boot_info.h"

// Basic memcmp/memcpy for freestanding env if needed
static size_t my_strlen(const char *str) {
    size_t len = 0;
    while (str && str[len] != '\0') {
        len++;
    }
    return len;
}

static int fdt_str_eq_local(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { a++; b++; }
    return (*a == *b);
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

    bi->selected_mode = BOOT_MODE_NORMAL;
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

typedef struct {
    uint32_t magic;
    uint32_t abi_version;
    uint32_t header_size;
    uint32_t module_kind;
    uint32_t payload_offset;
    uint32_t payload_size;
    uint32_t target_arch;
    uint32_t elf_class;
    uint32_t flags;
    uint32_t name_length;
    uint32_t digest_algorithm;
    uint8_t payload_digest[32];
    char name[32];
    uint8_t padding[20];
} __attribute__((packed)) bh_boot_module_header_local_t;

_Static_assert(sizeof(bh_boot_module_header_local_t) == 128,
               "boot module header must match its versioned header_size");

int boot_info_finalize(boot_info_t *bi) {
    if (!bi || bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        return -1;
    }

    // Set canonical pointer width
#if defined(__x86_64__) || defined(__aarch64__) || (defined(__riscv) && __riscv_xlen == 64)
    bi->pointer_width = 64;
#else
    bi->pointer_width = 32;
#endif

    // Resolve memory model defaults
#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(BHARAT_ENABLE_MPU)
    bi->memory_model = BH_MEM_MODEL_MPU;
#elif defined(BHARAT_PROFILE_MMU_LITE)
    bi->memory_model = BH_MEM_MODEL_MMU_LITE;
#else
    bi->memory_model = BH_MEM_MODEL_MMU_FULL;
#endif

    // Set execution profile defaults
#if defined(BHARAT_KERNEL_PROFILE_RT)
    bi->execution_profile = 2; // RT
#elif defined(BHARAT_KERNEL_PROFILE_MIX)
    bi->execution_profile = 1; // MIX
#else
    bi->execution_profile = 0; // GP
#endif

    // Set device profile defaults
#if defined(BHARAT_PROFILE_DESKTOP)
    bi->device_profile = 1; // DESKTOP
#elif defined(BHARAT_PROFILE_RTOS)
    bi->device_profile = 2; // RTOS
#elif defined(BHARAT_PROFILE_EDGE)
    bi->device_profile = 3; // EDGE
#else
    bi->device_profile = 0;
#endif

    // Default CPU count
    bi->cpu_count = 1;

    // Process and interpret registered boot modules (including initrd/unwrapped)
    for (uint32_t i = 0; i < bi->module_count; i++) {
        uint64_t phys = bi->modules[i].phys_start;
        uint64_t size = bi->modules[i].size;

        if (size < 128) continue;

        // Obtain virtual pointer safely
        void *virt_ptr = NULL;
#ifdef BHARAT_HOST_TEST
        virt_ptr = (void *)(uintptr_t)phys;
#else
        extern void *physmap_phys_to_virt(uint64_t phys) __attribute__((weak));
        if (physmap_phys_to_virt) {
            virt_ptr = physmap_phys_to_virt(phys);
        } else {
            virt_ptr = (void *)(uintptr_t)phys;
        }
#endif

        if (!virt_ptr) continue;

        // Check if the module starts with the Bharat boot-module container magic (0xB4A2D1A5)
        const bh_boot_module_header_local_t *hdr = (const bh_boot_module_header_local_t *)virt_ptr;
        if (hdr->magic == 0xB4A2D1A5) {
            // Found a valid container! Parse and normalize it.
            if (hdr->header_size != 128 || hdr->payload_offset != 128 ||
                hdr->payload_offset + hdr->payload_size > size) {
                bi->is_degraded = true;
                bi->degraded_reasons_mask |= 0x1000;
                continue;
            }

            // Assign name from stable name field inside the header
            bi->modules[i].name = hdr->name;

            // Strip the 128-byte header so it points directly to the ELF payload!
            bi->modules[i].phys_start = phys + 128;
            bi->modules[i].size = hdr->payload_size;

            // Set direct payload metadata fields
            if (fdt_str_eq_local(hdr->name, "services/init") || fdt_str_eq_local(hdr->name, "services/rt-supervisor")) {
                bi->init_payload_phys = bi->modules[i].phys_start;
                bi->init_payload_size = bi->modules[i].size;
                if (fdt_str_eq_local(hdr->name, "services/rt-supervisor")) {
                    bi->init_payload_kind = BH_BOOT_HANDOFF_STATIC_RT;
                } else {
                    bi->init_payload_kind = BH_BOOT_HANDOFF_USER_ELF;
                }
            }
        } else {
            // Backwards-compatible raw module handling
            if (fdt_str_eq_local(bi->modules[i].name, "services/init") || fdt_str_eq_local(bi->modules[i].name, "initrd")) {
                bi->init_payload_phys = phys;
                bi->init_payload_size = size;
                bi->init_payload_kind = BH_BOOT_HANDOFF_USER_ELF;
                // Normalize its name to canonical "services/init"
                bi->modules[i].name = "services/init";
            }
        }
    }

    return 0;
}
