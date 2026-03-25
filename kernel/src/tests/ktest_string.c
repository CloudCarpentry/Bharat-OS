#include <lib/base/string.h>
#include <tests/ktest.h>
#include <stdint.h>
#include <stddef.h>

static int test_memcpy(void) {
    char src[] = "hello";
    char dest[6] = {0};

    memcpy(dest, src, sizeof(src));
    if (strcmp(dest, "hello") != 0) {
        return -1;
    }
    return 0;
}

static int test_memset(void) {
    char buf[10];

    memset(buf, 'A', sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        if (buf[i] != 'A') {
            return -1;
        }
    }
    return 0;
}

static int test_memcmp(void) {
    char s1[] = "apple";
    char s2[] = "apple";
    char s3[] = "applz";

    if (memcmp(s1, s2, 5) != 0) return -1;
    if (memcmp(s1, s3, 5) >= 0) return -2;
    if (memcmp(s3, s1, 5) <= 0) return -3;

    return 0;
}

static int test_strlen(void) {
    if (strlen("hello") != 5) return -1;
    if (strlen("") != 0) return -2;
    return 0;
}

static int test_strcmp(void) {
    if (strcmp("hello", "hello") != 0) return -1;
    if (strcmp("hello", "world") >= 0) return -2;
    if (strcmp("world", "hello") <= 0) return -3;
    if (strcmp("hello", "hell") <= 0) return -4;
    return 0;
}

static int test_strncmp(void) {
    if (strncmp("hello", "hello", 5) != 0) return -1;
    if (strncmp("hello", "helloworld", 5) != 0) return -2;
    if (strncmp("hello", "hellz", 4) != 0) return -3;
    if (strncmp("hello", "hellz", 5) >= 0) return -4;
    return 0;
}

static int test_strcpy(void) {
    char src[] = "test";
    char dest[10];

    strcpy(dest, src);
    if (strcmp(dest, "test") != 0) return -1;
    return 0;
}

static int test_strncpy(void) {
    char src[] = "testing";
    char dest[10];

    /* normal case */
    strncpy(dest, src, 4);
    dest[4] = '\0';
    if (strcmp(dest, "test") != 0) return -1;

    /* null padding case */
    strncpy(dest, "hi", 5);
    if (dest[0] != 'h' || dest[1] != 'i' || dest[2] != '\0' || dest[3] != '\0' || dest[4] != '\0') {
        return -2;
    }
    return 0;
}

static int test_secure_memzero(void) {
    uint8_t buf[16];

    /* fill with noise */
    for (int i = 0; i < 16; i++) {
        buf[i] = (uint8_t)i;
    }

    secure_memzero(buf, sizeof(buf));

    /* verify it's zeroed */
    for (int i = 0; i < 16; i++) {
        if (buf[i] != 0) return -1;
    }
    return 0;
}

static int ktest_string_lib_run(void) {
    if (test_memcpy() != 0) return -1;
    if (test_memset() != 0) return -2;
    if (test_memcmp() != 0) return -3;
    if (test_strlen() != 0) return -4;
    if (test_strcmp() != 0) return -5;
    if (test_strncmp() != 0) return -6;
    if (test_strcpy() != 0) return -7;
    if (test_strncpy() != 0) return -8;
    if (test_secure_memzero() != 0) return -9;

    return 0;
}

REGISTER_BOOT_SELFTEST("string_lib", "lib", ktest_string_lib_run, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
