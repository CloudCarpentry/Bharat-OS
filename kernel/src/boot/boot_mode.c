#include "boot/boot_mode.h"
#include "boot/boot_args.h"
#include "console/console_core.h"
#include <stddef.h>

const char *bharat_boot_mode_name(bharat_boot_mode_t mode) {
    switch (mode) {
        case BHARAT_BOOT_MODE_NORMAL: return "normal";
        case BHARAT_BOOT_MODE_DIAGNOSTIC: return "diagnostic";
        case BHARAT_BOOT_MODE_RECOVERY: return "recovery";
        case BHARAT_BOOT_MODE_MANUFACTURING: return "manufacturing";
        case BHARAT_BOOT_MODE_BENCHMARK: return "benchmark";
        case BHARAT_BOOT_MODE_LEGACY_BRINGUP: return "legacy_bringup";
        default: return "unknown";
    }
}

static bool string_equals(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return false;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

bharat_boot_mode_t bharat_boot_mode_select(void) {
    char mode_str[32] = {0};

    if (boot_get_kv("mode", mode_str, sizeof(mode_str))) {
        if (string_equals(mode_str, "normal")) return BHARAT_BOOT_MODE_NORMAL;
        if (string_equals(mode_str, "diag") || string_equals(mode_str, "diagnostic")) return BHARAT_BOOT_MODE_DIAGNOSTIC;
        if (string_equals(mode_str, "recovery")) return BHARAT_BOOT_MODE_RECOVERY;
        if (string_equals(mode_str, "manufacturing")) return BHARAT_BOOT_MODE_MANUFACTURING;
        if (string_equals(mode_str, "benchmark")) return BHARAT_BOOT_MODE_BENCHMARK;
        if (string_equals(mode_str, "legacy") || string_equals(mode_str, "bringup")) return BHARAT_BOOT_MODE_LEGACY_BRINGUP;
    }

    // Fallback logic
#if defined(DEBUG) || defined(BHARAT_BOOT_BRINGUP)
    return BHARAT_BOOT_MODE_LEGACY_BRINGUP;
#else
    return BHARAT_BOOT_MODE_LEGACY_BRINGUP; // Default to legacy bringup for safety in this patch based on user prompt fallback behavior
#endif
}
