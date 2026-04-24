#include <stdio.h>
#include <assert.h>
#include <bharat/runtime/freestanding_string.h>

void test_memset() {
    char buf[10];
    memset(buf, 'A', sizeof(buf));
    for (int i = 0; i < 10; i++) {
        assert(buf[i] == 'A');
    }
}

void test_memcpy() {
    char src[] = "hello";
    char dst[10] = {0};
    memcpy(dst, src, 6); // including null terminator
    assert(dst[0] == 'h');
    assert(dst[1] == 'e');
    assert(dst[2] == 'l');
    assert(dst[3] == 'l');
    assert(dst[4] == 'o');
    assert(dst[5] == '\0');
}

void test_memcmp() {
    char a[] = "test";
    char b[] = "test";
    char c[] = "tess";

    assert(memcmp(a, b, 4) == 0);
    assert(memcmp(a, c, 4) > 0);
    assert(memcmp(c, a, 4) < 0);
}

void test_strlen() {
    char str1[] = "hello";
    char str2[] = "";
    assert(strlen(str1) == 5);
    assert(strlen(str2) == 0);
}

void test_strncpy() {
    char dst[10] = {0};
    strncpy(dst, "hi", 5);
    assert(dst[0] == 'h');
    assert(dst[1] == 'i');
    assert(dst[2] == '\0');
    assert(dst[3] == '\0'); // null padding check
}

void test_strncmp() {
    char a[] = "hello";
    char b[] = "helpa";
    assert(strncmp(a, b, 3) == 0);
    assert(strncmp(a, b, 4) < 0);
}

int main() {
    printf("Running freestanding string tests...\n");
    test_memset();
    test_memcpy();
    test_memcmp();
    test_strlen();
    test_strncpy();
    test_strncmp();
    printf("All freestanding string tests passed.\n");
    return 0;
}
