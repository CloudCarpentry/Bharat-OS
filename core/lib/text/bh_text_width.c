#include "bh_utf8.h"

int bh_text_cell_width(uint32_t cp) {
    // ASCII control characters
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) {
        return 0;
    }

    // Combining marks for Indian scripts (simplified check)
    if (cp >= 0x0900 && cp <= 0x0D7F) {
        // Devanagari (0900-097F)
        // Bengali (0980-09FF)
        // Gurmukhi (0A00-0A7F)
        // Gujarati (0A80-0AFF)
        // Tamil (0B00-0B7F - Odia also here)
        // Telugu (0C00-0C7F)
        // Kannada (0C80-0CFF)
        // Malayalam (0D00-0D7F)

        // This is a coarse filter for combining marks.
        // In a full implementation, we'd have a bitmask or range table.
        // For now, we identify common non-spacing/combining ranges.

        // Example: Devanagari signs/vowels that are non-spacing
        if (cp == 0x0900 || cp == 0x0901 || cp == 0x0902) return 0; // Signs
        if (cp >= 0x093A && cp <= 0x0940) return 0; // Vowel signs
        if (cp >= 0x0941 && cp <= 0x0948) return 0; // Vowel signs
        if (cp == 0x094D) return 0; // Virama

        // General rule for Indian scripts: many vowels and signs are combining.
        // For a minimal first version, we can use the Unicode category:
        // Mn (Mark, Nonspacing) usually have width 0.
        // For simplicity, we just handle the most obvious ones or return 0
        // for known mark ranges.

        // If it's a known mark range within the script:
        // (This is still very rough, but better than width 1)
        uint32_t script_offset = cp % 0x80;
        if (script_offset < 0x04) return 0; // Signs
        if (script_offset >= 0x3E && script_offset <= 0x4D) return 0; // Vowel signs / Virama
    }

    // Other combining marks (Generic)
    if (cp >= 0x0300 && cp <= 0x036F) return 0; // Combining Diacritical Marks
    if (cp >= 0x1DC0 && cp <= 0x1DFF) return 0; // Combining Diacritical Marks Supplement
    if (cp >= 0x20D0 && cp <= 0x20FF) return 0; // Combining Diacritical Marks for Symbols
    if (cp >= 0xFE20 && cp <= 0xFE2F) return 0; // Combining Half Marks

    // Default for most printable characters
    return 1;
}
