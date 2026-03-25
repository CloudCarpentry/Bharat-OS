#include <stdio.h>

int main() {
    unsigned int v1 = (1U << 31);
    unsigned long long v2 = (1ULL << 32);
    printf("1U << 31 = %u\n", v1);
    printf("1ULL << 32 = %llu\n", v2);
    return 0;
}
