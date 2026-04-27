#include "bh_utf8.h"

size_t bh_text_sanitize_console(const char *in, size_t in_len, char *out, size_t out_cap) {
    if (!in || !out || out_cap == 0) return 0;

    size_t in_off = 0;
    size_t out_off = 0;
    uint32_t cp;

    while (in_off < in_len && out_off < out_cap) {
        bh_utf8_status_t status = bh_utf8_next(in, in_len, &in_off, &cp);

        if (status != BH_UTF8_OK) {
            // Replace invalid sequence with '?' or replacement glyph
            if (out_off < out_cap) {
                out[out_off++] = '?';
            }
            // Manual advance to avoid infinite loop on invalid/overlong/surrogate/out-of-range
            // or truncated sequences.
            in_off++;
            continue;
        }

        // Strip unsafe escape sequences (simplified: strip all ESC)
        if (cp == 0x1B) {
            continue;
        }

        // Allow most printable characters and common whitespace
        if (cp >= 0x20 || cp == '\n' || cp == '\r' || cp == '\t') {
            char tmp[4];
            size_t n = bh_utf8_encode(cp, tmp);
            if (out_off + n <= out_cap) {
                for (size_t i = 0; i < n; i++) {
                    out[out_off++] = tmp[i];
                }
            } else {
                break; // Out of space
            }
        } else {
            // Strip other C0 controls
            continue;
        }
    }

    return out_off;
}
