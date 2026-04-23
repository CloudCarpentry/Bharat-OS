#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_memcpy_basic(void) {
    const uint8_t src[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    uint8_t dst[8] = {0};

    memcpy(dst, src, sizeof(src));
    return (memcmp(dst, src, sizeof(src)) == 0) ? 0 : 1;
}

static int test_memset_basic(void) {
    uint8_t buf[16];
    memset(buf, 0xA5, sizeof(buf));

    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] != 0xA5) {
            return 1;
        }
    }
    return 0;
}

static int test_memmove_overlap_forward(void) {
    char buf[8] = {'a', 'b', 'c', 'd', 'e', 'f', '\0', '\0'};
    memmove(buf + 2, buf, 4);
    return (memcmp(buf, "ababcd", 6) == 0) ? 0 : 1;
}

static int test_memmove_overlap_backward(void) {
    char buf[8] = {'a', 'b', 'c', 'd', 'e', 'f', '\0', '\0'};
    memmove(buf, buf + 2, 4);
    return (memcmp(buf, "cdef", 4) == 0) ? 0 : 1;
}

static int test_strlen_strcmp(void) {
    if (strlen("bharat") != 6) {
        return 1;
    }
    if (strcmp("bharat", "bharat") != 0) {
        return 1;
    }
    if (strcmp("bharat", "bharati") >= 0) {
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    failures += test_memcpy_basic();
    failures += test_memset_basic();
    failures += test_memmove_overlap_forward();
    failures += test_memmove_overlap_backward();
    failures += test_strlen_strcmp();

    if (failures != 0) {
        fprintf(stderr, "test_lib_string: %d checks failed\n", failures);
        return 1;
    }

    return 0;
}
