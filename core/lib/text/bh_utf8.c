#include "bh_utf8.h"

bh_utf8_status_t bh_utf8_next(const char *s, size_t len, size_t *off, uint32_t *cp) {
    if (!s || !off || !cp || *off >= len) {
        return BH_UTF8_ERR_INVALID;
    }

    const uint8_t *u = (const uint8_t *)s;
    size_t i = *off;
    uint8_t c = u[i];

    if (c <= 0x7F) {
        *cp = c;
        *off = i + 1;
        return BH_UTF8_OK;
    }

    uint32_t res;
    int count;

    if ((c & 0xE0) == 0xC0) {
        res = c & 0x1F;
        count = 1;
    } else if ((c & 0xF0) == 0xE0) {
        res = c & 0x0F;
        count = 2;
    } else if ((c & 0xF8) == 0xF0) {
        res = c & 0x07;
        count = 3;
    } else {
        return BH_UTF8_ERR_INVALID;
    }

    if (i + count >= len) {
        return BH_UTF8_ERR_TRUNCATED;
    }

    for (int j = 1; j <= count; j++) {
        uint8_t next = u[i + j];
        if ((next & 0xC0) != 0x80) return BH_UTF8_ERR_INVALID;
        res = (res << 6) | (next & 0x3F);
    }

    // Check for overlong encodings
    if (count == 1 && res < 0x80) return BH_UTF8_ERR_OVERLONG;
    if (count == 2 && res < 0x800) return BH_UTF8_ERR_OVERLONG;
    if (count == 3 && res < 0x10000) return BH_UTF8_ERR_OVERLONG;

    // Check for surrogates
    if (res >= 0xD800 && res <= 0xDFFF) return BH_UTF8_ERR_SURROGATE;

    // Check for out of range
    if (res > 0x10FFFF) return BH_UTF8_ERR_OUT_OF_RANGE;

    *cp = res;
    *off = i + count + 1;
    return BH_UTF8_OK;
}

bh_utf8_status_t bh_utf8_validate(const char *s, size_t len) {
    size_t off = 0;
    uint32_t cp;
    while (off < len) {
        bh_utf8_status_t status = bh_utf8_next(s, len, &off, &cp);
        if (status != BH_UTF8_OK) return status;
    }
    return BH_UTF8_OK;
}

size_t bh_utf8_encode(uint32_t cp, char out[4]) {
    uint8_t *u = (uint8_t *)out;
    if (cp <= 0x7F) {
        u[0] = (uint8_t)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        u[0] = (0xC0 | (uint8_t)(cp >> 6));
        u[1] = (0x80 | (uint8_t)(cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        if (cp >= 0xD800 && cp <= 0xDFFF) return 0;
        u[0] = (0xE0 | (uint8_t)(cp >> 12));
        u[1] = (0x80 | (uint8_t)((cp >> 6) & 0x3F));
        u[2] = (0x80 | (uint8_t)(cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        u[0] = (0xF0 | (uint8_t)(cp >> 18));
        u[1] = (0x80 | (uint8_t)((cp >> 12) & 0x3F));
        u[2] = (0x80 | (uint8_t)((cp >> 6) & 0x3F));
        u[3] = (0x80 | (uint8_t)(cp & 0x3F));
        return 4;
    }
    return 0;
}
