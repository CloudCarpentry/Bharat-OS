#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "core/lib/text/bh_utf8.h"

void test_valid_utf8() {
    printf("Testing valid UTF-8...\n");
    const char *s = "Hello, \xe0\xa4\xad\xe0\xa4\xbe\xe0\xa4\xb0\xe0\xa4\xa4!"; // Hello, Bharat (in Devanagari)
    assert(bh_utf8_validate(s, strlen(s)) == BH_UTF8_OK);

    size_t off = 0;
    uint32_t cp;
    size_t len = strlen(s);

    // 'H'
    assert(bh_utf8_next(s, len, &off, &cp) == BH_UTF8_OK);
    assert(cp == 'H');
    assert(off == 1);

    // skip "ello, "
    off = 7;

    // Devanagari BHA (U+092D) -> \xe0\xa4\xad
    assert(bh_utf8_next(s, len, &off, &cp) == BH_UTF8_OK);
    assert(cp == 0x092D);
    assert(off == 10);
}

void test_invalid_utf8() {
    printf("Testing invalid UTF-8...\n");

    // Overlong encoding for 'A' (0x41)
    const char *overlong = "\xc1\x81";
    assert(bh_utf8_validate(overlong, 2) == BH_UTF8_ERR_OVERLONG);

    // Surrogate range
    const char *surrogate = "\xed\xa0\x80"; // U+D800
    assert(bh_utf8_validate(surrogate, 3) == BH_UTF8_ERR_SURROGATE);

    // Out of range
    const char *out_of_range = "\xf4\x90\x80\x80"; // U+110000
    assert(bh_utf8_validate(out_of_range, 4) == BH_UTF8_ERR_OUT_OF_RANGE);

    // Truncated
    const char *truncated = "\xe0\xa4";
    assert(bh_utf8_validate(truncated, 2) == BH_UTF8_ERR_TRUNCATED);
}

void test_cell_width() {
    printf("Testing cell width...\n");
    assert(bh_text_cell_width('A') == 1);
    assert(bh_text_cell_width(0x0902) == 0); // Devanagari Sign Anusvara
    assert(bh_text_cell_width(0x0941) == 0); // Devanagari Vowel Sign U
    assert(bh_text_cell_width(0x092D) == 1); // Devanagari BHA
}

void test_sanitize() {
    printf("Testing sanitization...\n");
    // Use separate literals to avoid \x consuming too many characters
    const char *unsafe = "Normal\x1b" "[31mRed\x07" "Alert";
    char out[64];
    size_t n = bh_text_sanitize_console(unsafe, strlen(unsafe), out, sizeof(out));
    out[n] = '\0';
    printf("Sanitized: '");
    for (size_t i = 0; i < n; i++) {
        if (out[i] < 32) printf("\\x%02x", (unsigned char)out[i]);
        else printf("%c", out[i]);
    }
    printf("'\n");
    // It strips ESC (0x1B) and BEL (0x07)
    // The expected string without ESC and BEL is "Normal[31mRedAlert"
    if (strcmp(out, "Normal[31mRedAlert") != 0) {
        printf("FAILED: expected 'Normal[31mRedAlert', got '%s'\n", out);
        exit(1);
    }
}

void test_regression_boundary() {
    printf("Testing C literal hex boundary regression...\n");
    const char *bad = "Red\x07Alert";
    const char *good = "Red\x07" "Alert";

    // bad actually contains 'z' because \x07A -> 0x7A -> 'z'
    assert(strchr(bad, 'z') != NULL);
    assert(strchr(good, 'z') == NULL);
}

int main() {
    test_valid_utf8();
    test_invalid_utf8();
    test_cell_width();
    test_sanitize();
    test_regression_boundary();
    printf("All tests passed!\n");
    return 0;
}
