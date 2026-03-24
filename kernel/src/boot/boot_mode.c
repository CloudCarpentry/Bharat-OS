#include "boot/boot_mode.h"
#include "boot/boot_args.h"
#include "console/console_core.h"
#include <boot/boot_info.h>
#include <stddef.h>

const char *bharat_boot_mode_name(bharat_boot_mode_t mode) {
    switch (mode) {
        case BOOT_MODE_NORMAL: return "normal";
        case BOOT_MODE_DEBUG: return "debug";
        case BOOT_MODE_SELFTEST: return "selftest";
        case BOOT_MODE_RECOVERY: return "recovery";
        case BOOT_MODE_SAFE: return "safe";
        case BOOT_MODE_PROVISIONING: return "provisioning";
        case BOOT_MODE_BENCHMARK: return "benchmark";
        case BOOT_MODE_LEGACY_BRINGUP: return "legacy_bringup";
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

// Minimal token finder for canonical boot info string parsing
static bool cmdline_has_kv(const char *cmdline, const char *key, char *out, size_t out_sz) {
    if (!cmdline || !key || !out) return false;

    // Very simple linear scan for "key=value"
    // Just a placeholder for testing without a full generic parser
    const char *p = cmdline;
    while (*p) {
        // Skip space
        while (*p == ' ') p++;
        if (!*p) break;

        const char *start = p;
        while (*p && *p != '=' && *p != ' ') p++;

        size_t key_len = p - start;
        bool match = true;
        for (size_t i = 0; i < key_len; i++) {
            if (key[i] != start[i]) {
                match = false;
                break;
            }
        }
        if (key[key_len] != '\0') match = false;

        if (match && *p == '=') {
            p++;
            const char *vstart = p;
            while (*p && *p != ' ') p++;
            size_t vlen = p - vstart;
            if (vlen >= out_sz) vlen = out_sz - 1;
            for (size_t i = 0; i < vlen; i++) {
                out[i] = vstart[i];
            }
            out[vlen] = '\0';
            return true;
        }

        while (*p && *p != ' ') p++;
    }
    return false;
}

int boot_mode_resolve(const struct boot_info *bi, bharat_boot_mode_t *out_mode) {
    if (!bi || !out_mode) return -1;

    char mode_str[32] = {0};

    // If canonical cmdline parsing works, prefer it
    if (cmdline_has_kv(bi->cmdline, "mode", mode_str, sizeof(mode_str))) {
        if (string_equals(mode_str, "normal")) { *out_mode = BOOT_MODE_NORMAL; return 0; }
        if (string_equals(mode_str, "debug") || string_equals(mode_str, "diagnostic")) { *out_mode = BOOT_MODE_DEBUG; return 0; }
        if (string_equals(mode_str, "selftest")) { *out_mode = BOOT_MODE_SELFTEST; return 0; }
        if (string_equals(mode_str, "recovery")) { *out_mode = BOOT_MODE_RECOVERY; return 0; }
        if (string_equals(mode_str, "safe")) { *out_mode = BOOT_MODE_SAFE; return 0; }
        if (string_equals(mode_str, "manufacturing") || string_equals(mode_str, "provisioning")) { *out_mode = BOOT_MODE_PROVISIONING; return 0; }
        if (string_equals(mode_str, "benchmark")) { *out_mode = BOOT_MODE_BENCHMARK; return 0; }
        if (string_equals(mode_str, "legacy") || string_equals(mode_str, "bringup")) { *out_mode = BOOT_MODE_LEGACY_BRINGUP; return 0; }
    }

    // Fallback logic
#if defined(DEBUG) || defined(BHARAT_BOOT_BRINGUP)
    *out_mode = BOOT_MODE_LEGACY_BRINGUP;
#else
    *out_mode = BOOT_MODE_NORMAL;
#endif
    return 0;
}

bool boot_mode_allows_selftests(bharat_boot_mode_t mode, const struct boot_info *bi) {
    (void)bi; // Can extend with policy bits in the future
    return mode == BOOT_MODE_DEBUG ||
           mode == BOOT_MODE_SELFTEST ||
           mode == BOOT_MODE_PROVISIONING ||
           mode == BOOT_MODE_LEGACY_BRINGUP;
}

bool boot_mode_allows_debug_args(bharat_boot_mode_t mode, const struct boot_info *bi) {
    (void)bi;
    return mode == BOOT_MODE_DEBUG || mode == BOOT_MODE_LEGACY_BRINGUP;
}

bool boot_mode_should_skip_optional_drivers(bharat_boot_mode_t mode) {
    return mode == BOOT_MODE_RECOVERY || mode == BOOT_MODE_SAFE;
}

bharat_boot_mode_t bharat_boot_mode_select(void) {
    char mode_str[32] = {0};

    if (boot_get_kv("mode", mode_str, sizeof(mode_str))) {
        if (string_equals(mode_str, "normal")) return BOOT_MODE_NORMAL;
        if (string_equals(mode_str, "debug") || string_equals(mode_str, "diagnostic")) return BOOT_MODE_DEBUG;
        if (string_equals(mode_str, "selftest")) return BOOT_MODE_SELFTEST;
        if (string_equals(mode_str, "recovery")) return BOOT_MODE_RECOVERY;
        if (string_equals(mode_str, "safe")) return BOOT_MODE_SAFE;
        if (string_equals(mode_str, "manufacturing") || string_equals(mode_str, "provisioning")) return BOOT_MODE_PROVISIONING;
        if (string_equals(mode_str, "benchmark")) return BOOT_MODE_BENCHMARK;
        if (string_equals(mode_str, "legacy") || string_equals(mode_str, "bringup")) return BOOT_MODE_LEGACY_BRINGUP;
    }

    // Fallback logic
#if defined(DEBUG) || defined(BHARAT_BOOT_BRINGUP)
    return BOOT_MODE_LEGACY_BRINGUP;
#else
    return BOOT_MODE_LEGACY_BRINGUP; // Default to legacy bringup for safety in this patch based on user prompt fallback behavior
#endif
}
