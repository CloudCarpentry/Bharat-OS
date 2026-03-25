#include <stdio.h>

typedef enum {
    TEST_1 = (1U << 31),
    TEST_2 = (1ULL << 32),
} test_enum_t;

int main() {
    printf("TEST_1 = %u\n", TEST_1);
    return 0;
}
