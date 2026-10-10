#include <bharat/cap/cap.h>
#include <stdio.h>
#include <string.h>

int tests_failed = 0;

#define EXPECT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            printf("FAIL: Expected '%s', got '%s' at %s:%d\n", \
                   (expected), (actual), __FILE__, __LINE__); \
            tests_failed++; \
        } \
    } while (0)

#define EXPECT_INT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            printf("FAIL: Expected %d, got %d at %s:%d\n", \
                   (int)(expected), (int)(actual), __FILE__, __LINE__); \
            tests_failed++; \
        } \
    } while (0)

void test_bharat_cap_format() {
    char buf[64];

    // Test valid handles
    bharat_cap_format(0x12345678, buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "0x0000000012345678");

    bharat_cap_format(0xABCDEF0123456789ULL, buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "0xabcdef0123456789");

    // Test invalid handle
    bharat_cap_format(BHARAT_CAP_INVALID_HANDLE, buf, sizeof(buf));
    EXPECT_STR_EQ(buf, "0x0000000000000000");

    // Test small buffer (exact size: 19 bytes)
    char exact_buf[19];
    bharat_cap_format(0x1234567890ABCDEFULL, exact_buf, sizeof(exact_buf));
    EXPECT_STR_EQ(exact_buf, "0x1234567890abcdef");
    EXPECT_INT_EQ(strlen(exact_buf), 18);

    // Test too small buffer
    char tiny_buf[5];
    bharat_cap_format(0x12345678, tiny_buf, sizeof(tiny_buf));
    EXPECT_STR_EQ(tiny_buf, "0x00"); // 4 characters + null
    EXPECT_INT_EQ(strlen(tiny_buf), 4);

    char medium_buf[11];
    bharat_cap_format(0x12345678, medium_buf, sizeof(medium_buf));
    EXPECT_STR_EQ(medium_buf, "0x00000000"); // 10 characters + null
    EXPECT_INT_EQ(strlen(medium_buf), 10);

    // Test zero capacity (should not crash or write)
    char zero_buf[5] = "test";
    bharat_cap_format(0x12345678, zero_buf, 0);
    EXPECT_STR_EQ(zero_buf, "test"); // unchanged

    // Test null buffer (should not crash)
    bharat_cap_format(0x12345678, NULL, 64);

    // Test len=1
    char one_buf[5] = "test";
    bharat_cap_format(0x12345678, one_buf, 1);
    EXPECT_STR_EQ(one_buf, "");
}

int main() {
    printf("Running cap tests...\n");
    test_bharat_cap_format();

    if (tests_failed == 0) {
        printf("All cap tests passed.\n");
        return 0;
    } else {
        printf("%d tests failed.\n", tests_failed);
        return 1;
    }
}
