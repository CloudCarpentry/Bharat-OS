#include "boot/boot_validate.h"
#include <stddef.h>

static void set_report(boot_validation_report_t *report, int code, const char *msg, uint32_t index, bool is_fatal) {
    if (report) {
        report->error_code = code;
        report->message = msg;
        report->failing_index = index;
        report->is_fatal = is_fatal;
    }
}

int boot_validate_basic(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) {
        set_report(report, BOOT_ERR_INVALID_ARGUMENT, "Null boot_info pointer", 0, true);
        return BOOT_ERR_INVALID_ARGUMENT;
    }

    if (bi->magic != BHARAT_BOOT_INFO_MAGIC) {
        set_report(report, BOOT_ERR_BAD_MAGIC, "Invalid magic number", 0, true);
        return BOOT_ERR_BAD_MAGIC;
    }

    if (bi->mem_region_count > BHARAT_BOOT_MAX_MEM_REGIONS) {
        set_report(report, BOOT_ERR_TOO_MANY_MEM_REGIONS, "Exceeded max memory regions", 0, true);
        return BOOT_ERR_TOO_MANY_MEM_REGIONS;
    }

    if (bi->module_count > BHARAT_BOOT_MAX_MODULES) {
        set_report(report, BOOT_ERR_TOO_MANY_MODULES, "Exceeded max modules", 0, true);
        return BOOT_ERR_TOO_MANY_MODULES;
    }

    // Checking string termination on cmdline
    bool terminated = false;
    for (size_t i = 0; i < BHARAT_BOOT_CMDLINE_MAX_LEN; i++) {
        if (bi->cmdline[i] == '\0') {
            terminated = true;
            break;
        }
    }
    if (!terminated) {
        set_report(report, BOOT_ERR_CMDLINE_TOO_LONG, "Cmdline string missing null terminator", 0, true);
        return BOOT_ERR_CMDLINE_TOO_LONG;
    }

    return BOOT_OK;
}

int boot_validate_memory_map(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) return BOOT_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < bi->mem_region_count; i++) {
        const boot_memory_region_t *reg = &bi->mem_regions[i];
        if (reg->size == 0) {
            set_report(report, BOOT_ERR_INVALID_MEM_RANGE, "Memory region has zero size", i, true);
            return BOOT_ERR_INVALID_MEM_RANGE;
        }

        uint64_t end = reg->phys_start + reg->size;
        if (end < reg->phys_start) {
            set_report(report, BOOT_ERR_INVALID_MEM_RANGE, "Memory region wraparound", i, true);
            return BOOT_ERR_INVALID_MEM_RANGE;
        }

        // We could implement N^2 overlap check here, but realistically we expect
        // the map to either be sorted and non-overlapping, or we sort it and check.
        // For now, let's do a simple N^2 check.
        for (uint32_t j = i + 1; j < bi->mem_region_count; j++) {
            const boot_memory_region_t *other = &bi->mem_regions[j];
            uint64_t other_end = other->phys_start + other->size;

            if (reg->phys_start < other_end && other->phys_start < end) {
                set_report(report, BOOT_ERR_OVERLAPPING_MEM_RANGE, "Memory regions overlap", i, true);
                return BOOT_ERR_OVERLAPPING_MEM_RANGE;
            }
        }
    }

    // Validate kernel image region is reasonable
    if (bi->kernel_phys_end > 0) {
        if (bi->kernel_phys_start >= bi->kernel_phys_end) {
            set_report(report, BOOT_ERR_INVALID_MEM_RANGE, "Kernel start >= kernel end", 0, true);
            return BOOT_ERR_INVALID_MEM_RANGE;
        }
    }

    return BOOT_OK;
}

int boot_validate_modules(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) return BOOT_ERR_INVALID_ARGUMENT;

    for (uint32_t i = 0; i < bi->module_count; i++) {
        const boot_module_t *mod = &bi->modules[i];

        if (mod->size == 0) {
            set_report(report, BOOT_ERR_INVALID_MODULE, "Module has zero size", i, true);
            return BOOT_ERR_INVALID_MODULE;
        }

        uint64_t end = mod->phys_start + mod->size;
        if (end < mod->phys_start) {
            set_report(report, BOOT_ERR_INVALID_MODULE, "Module wraparound", i, true);
            return BOOT_ERR_INVALID_MODULE;
        }
    }

    return BOOT_OK;
}

int boot_validate_console(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) return BOOT_ERR_INVALID_ARGUMENT;

    if (bi->console.type == BOOT_CONSOLE_FRAMEBUFFER) {
        if (bi->console.fb_width == 0 || bi->console.fb_height == 0 || bi->console.fb_bpp == 0) {
            set_report(report, BOOT_ERR_CONSOLE_INVALID, "Invalid framebuffer geometry", 0, false);
            return BOOT_ERR_CONSOLE_INVALID;
        }
    }

    return BOOT_OK;
}

int boot_validate_firmware(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) return BOOT_ERR_INVALID_ARGUMENT;

    if (bi->firmware.fdt_ptr != NULL) {
        // Here we could validate FDT header magic if we pulled in libfdt or parsed it.
        // FDT magic: 0xd00dfeed
        uint32_t *fdt_magic = (uint32_t *)bi->firmware.fdt_ptr;
        // Basic check assuming little endian runtime and big endian FDT magic, or just the raw bytes:
        // Not doing byte-swap here for portability, but typically 0xd00dfeed in BE is 0xedfe0dd0 in LE
        // For a rigorous check, we just ensure it's not a null deref (we checked != NULL)
        (void)fdt_magic;
    }

    if (bi->firmware.has_rng_seed && bi->firmware.rng_seed_size == 0) {
        set_report(report, BOOT_ERR_FDT_INVALID, "RNG seed present but size 0", 0, false);
        return BOOT_ERR_FDT_INVALID; // non-fatal
    }

    return BOOT_OK;
}

int boot_validate_security(const boot_info_t *bi, boot_validation_report_t *report) {
    if (!bi) return BOOT_ERR_INVALID_ARGUMENT;

    // Reject combinations:
    if (bi->security_info.secure_boot_verified && !bi->security_info.secure_boot_present) {
        set_report(report, BOOT_ERR_SECURITY_POLICY, "Secure boot verified but not present", 0, true);
        return BOOT_ERR_SECURITY_POLICY;
    }

    return BOOT_OK;
}

int boot_validate_all(boot_info_t *bi, boot_validation_report_t *report) {
    int ret;

    if ((ret = boot_validate_basic(bi, report)) != BOOT_OK) return ret;
    if ((ret = boot_validate_memory_map(bi, report)) != BOOT_OK) return ret;
    if ((ret = boot_validate_modules(bi, report)) != BOOT_OK) return ret;

    // Non-fatal validations, mark degraded if they fail
    ret = boot_validate_console(bi, report);
    if (ret != BOOT_OK) {
        bi->is_degraded = true;
        bi->degraded_reasons_mask |= (1 << ( -ret ));
    }

    ret = boot_validate_firmware(bi, report);
    if (ret != BOOT_OK) {
        bi->is_degraded = true;
        bi->degraded_reasons_mask |= (1 << ( -ret ));
    }

    if ((ret = boot_validate_security(bi, report)) != BOOT_OK) return ret;

    // Reset report if we succeeded overall
    if (report) {
        report->error_code = BOOT_OK;
        report->message = "Validation passed";
        report->is_fatal = false;
    }

    bi->is_validated = true;
    return BOOT_OK;
}
