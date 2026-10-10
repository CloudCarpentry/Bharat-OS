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

    // Null and empty strings
    assert(bh_utf8_validate(NULL, 1) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_validate(NULL, 0) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_validate("", 0) == BH_UTF8_OK);

    // Invalid leading and continuation bytes
    assert(bh_utf8_validate("\x80", 1) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_validate("\xC2\x7F", 2) == BH_UTF8_ERR_INVALID);

    // Offset behavior after decoding failure
    size_t off = 0;
    uint32_t cp = 0;
    assert(bh_utf8_next(NULL, 1, &off, &cp) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_next("a", 1, NULL, &cp) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_next("a", 1, &off, NULL) == BH_UTF8_ERR_INVALID);
    assert(bh_utf8_next("\xC2\x7F", 2, &off, &cp) == BH_UTF8_ERR_INVALID);
    assert(off == 0); // Offset should not advance on failure

    // Overlong encoding for 'A' (0x41)
    const char *overlong = "\xc1\x81";
    assert(bh_utf8_validate(overlong, 2) == BH_UTF8_ERR_OVERLONG);

    // Surrogate range
    const char *surrogate = "\xed\xa0\x80"; // U+D800
    assert(bh_utf8_validate(surrogate, 3) == BH_UTF8_ERR_SURROGATE);

    // Out of range
    const char *out_of_range = "\xf4\x90\x80\x80"; // U+110000
    assert(bh_utf8_validate(out_of_range, 4) == BH_UTF8_ERR_OUT_OF_RANGE);

    // Unicode Maximum U+10FFFF
    const char *unicode_max = "\xf4\x8f\xbf\xbf"; // U+10FFFF
    assert(bh_utf8_validate(unicode_max, 4) == BH_UTF8_OK);

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

void test_sanitize_regression() {
    printf("Testing sanitize regressions...\n");
    char out[64];
    size_t n;

    // Empty input
    n = bh_text_sanitize_console("", 0, out, sizeof(out));
    assert(n == 0);

    // Zero-capacity output
    n = bh_text_sanitize_console("abc", 3, out, 0);
    assert(n == 0);

    // Output buffer exactly at capacity
    memset(out, 0xAA, sizeof(out));
    n = bh_text_sanitize_console("abcde", 5, out, 5);
    assert(n == 5);
    assert(strncmp(out, "abcde", 5) == 0);
    assert((unsigned char)out[5] == 0xAA);

    // Truncated output
    memset(out, 0xAA, sizeof(out));
    n = bh_text_sanitize_console("abcdefgh", 8, out, 5);
    assert(n == 5);
    assert(strncmp(out, "abcde", 5) == 0);
    assert((unsigned char)out[5] == 0xAA);

    // Control characters (ESC, BEL, CR)
    memset(out, 0xAA, sizeof(out));
    const char *ctrl = "A\x1b" "B\x07" "C\r" "D";
    n = bh_text_sanitize_console(ctrl, strlen(ctrl), out, sizeof(out));
    assert(n == 5);
    assert(strncmp(out, "ABC\rD", 5) == 0);

    // Valid multibyte UTF-8 input
    memset(out, 0xAA, sizeof(out));
    const char *mb = "Hello, \xe0\xa4\xad";
    n = bh_text_sanitize_console(mb, strlen(mb), out, sizeof(out));
    assert(n == 10);
    assert(strncmp(out, "Hello, \xe0\xa4\xad", 10) == 0);
}

void test_regression_boundary() {
    printf("Testing C literal hex boundary regression...\n");
    const char *bad = "Red\x07Alert";
    const char *good = "Red\x07" "Alert";

    // bad actually contains 'z' because \x07A -> 0x7A -> 'z'
    assert(strchr(bad, 'z') != NULL);
    assert(strchr(good, 'z') == NULL);
}

void test_utf8_encode() {
    printf("Testing UTF-8 encoding...\n");
    char out[4];
    size_t len;

    // 1-byte encoding (ASCII)
    len = bh_utf8_encode('A', out);
    assert(len == 1);
    assert((uint8_t)out[0] == 'A');

    // 2-byte encoding (e.g., U+00E9 LATIN SMALL LETTER E WITH ACUTE)
    len = bh_utf8_encode(0x00E9, out);
    assert(len == 2);
    assert((uint8_t)out[0] == 0xC3);
    assert((uint8_t)out[1] == 0xA9);

    // 3-byte encoding (e.g., U+092D DEVANAGARI LETTER BHA)
    len = bh_utf8_encode(0x092D, out);
    assert(len == 3);
    assert((uint8_t)out[0] == 0xE0);
    assert((uint8_t)out[1] == 0xA4);
    assert((uint8_t)out[2] == 0xAD);

    // 4-byte encoding (e.g., U+1F600 GRINNING FACE)
    len = bh_utf8_encode(0x1F600, out);
    assert(len == 4);
    assert((uint8_t)out[0] == 0xF0);
    assert((uint8_t)out[1] == 0x9F);
    assert((uint8_t)out[2] == 0x98);
    assert((uint8_t)out[3] == 0x80);

    // Invalid encoding (Surrogates)
    len = bh_utf8_encode(0xD800, out);
    assert(len == 0);
    len = bh_utf8_encode(0xDFFF, out);
    assert(len == 0);

    // Invalid encoding (Out of range)
    len = bh_utf8_encode(0x110000, out);
    assert(len == 0);

    // Boundary: Max 1-byte
    len = bh_utf8_encode(0x7F, out);
    assert(len == 1);
    assert((uint8_t)out[0] == 0x7F);

    // Boundary: Min 2-byte
    len = bh_utf8_encode(0x80, out);
    assert(len == 2);
    assert((uint8_t)out[0] == 0xC2);
    assert((uint8_t)out[1] == 0x80);

    // Boundary: Max 2-byte
    len = bh_utf8_encode(0x7FF, out);
    assert(len == 2);
    assert((uint8_t)out[0] == 0xDF);
    assert((uint8_t)out[1] == 0xBF);

    // Boundary: Min 3-byte
    len = bh_utf8_encode(0x800, out);
    assert(len == 3);
    assert((uint8_t)out[0] == 0xE0);
    assert((uint8_t)out[1] == 0xA0);
    assert((uint8_t)out[2] == 0x80);

    // Boundary: Just before surrogate
    len = bh_utf8_encode(0xD7FF, out);
    assert(len == 3);
    assert((uint8_t)out[0] == 0xED);
    assert((uint8_t)out[1] == 0x9F);
    assert((uint8_t)out[2] == 0xBF);

    // Boundary: Just after surrogate
    len = bh_utf8_encode(0xE000, out);
    assert(len == 3);
    assert((uint8_t)out[0] == 0xEE);
    assert((uint8_t)out[1] == 0x80);
    assert((uint8_t)out[2] == 0x80);

    // Boundary: Max 3-byte
    len = bh_utf8_encode(0xFFFF, out);
    assert(len == 3);
    assert((uint8_t)out[0] == 0xEF);
    assert((uint8_t)out[1] == 0xBF);
    assert((uint8_t)out[2] == 0xBF);

    // Boundary: Min 4-byte
    len = bh_utf8_encode(0x10000, out);
    assert(len == 4);
    assert((uint8_t)out[0] == 0xF0);
    assert((uint8_t)out[1] == 0x90);
    assert((uint8_t)out[2] == 0x80);
    assert((uint8_t)out[3] == 0x80);

    // Boundary: Max 4-byte
    len = bh_utf8_encode(0x10FFFF, out);
    assert(len == 4);
    assert((uint8_t)out[0] == 0xF4);
    assert((uint8_t)out[1] == 0x8F);
    assert((uint8_t)out[2] == 0xBF);
    assert((uint8_t)out[3] == 0xBF);

    // Invalid encoding (Far out of range)
    len = bh_utf8_encode(0xFFFFFFFF, out);
    assert(len == 0);
}

int main() {
    test_valid_utf8();
    test_invalid_utf8();
    test_cell_width();
    test_sanitize();
    test_sanitize_regression();
    test_regression_boundary();
    test_utf8_encode();
    printf("All tests passed!\n");
    return 0;
}
