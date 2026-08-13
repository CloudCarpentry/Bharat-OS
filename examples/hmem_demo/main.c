#include <stdio.h>
#include <stdlib.h>

int main() {
    // Simple demo: allocate memory, write, and free.
    size_t size = 1024 * 1024; // 1 MiB
    void *buf = malloc(size);
    if (!buf) {
        perror("malloc");
        return 1;
    }
    // Touch the memory to ensure allocation.
    memset(buf, 0xAB, size);
    printf("Allocated %zu bytes and initialized.\n", size);
    free(buf);
    return 0;
}
