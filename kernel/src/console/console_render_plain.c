#include "console/console_render.h"
#include <stddef.h>

/* Console Plain Text Normalizer
 * Very simple generic stream normalization logic.
 * E.g., handling \n to \r\n conversion if needed, stripping ANSI codes, etc.
 * Currently, only passes plain text unaltered.
 */
void console_render_plain_write(const char *data, size_t len, void (*putc_cb)(char)) {
    if (!data || !putc_cb) return;

    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            putc_cb('\r');
        }
        putc_cb(data[i]);
    }
}
