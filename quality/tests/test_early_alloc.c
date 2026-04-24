#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Mock _end dynamically so valgrind works
uint8_t* _end_mock = NULL;
// The early_alloc code expects `_end` as an array symbol
// We will modify our test specifically to not rely on the `_end` symbol natively
// but pass our own start address to `early_alloc_init`.

// Overriding _end for the linker
uint8_t _end[1]; // Just to satisfy linker

#include "../kernel/src/mm/early_alloc.c"

int main(void) {
    _end_mock = malloc(409600); // 400KB
    assert(_end_mock != NULL);

    printf("Starting early alloc test\n");
    phys_addr_t start_addr = (phys_addr_t)(uintptr_t)_end_mock;
    printf("start_addr: %lx\n", (unsigned long)start_addr);

    // Explicitly initialize with our malloced buffer
    early_alloc_init(start_addr);

    printf("allocating 64 bytes\n");
    void* ptr1 = early_alloc(64, 8);
    printf("ptr1: %p\n", ptr1);
    assert(ptr1 != NULL);
    assert((uintptr_t)ptr1 % 8 == 0);

    printf("allocating 128 bytes\n");
    void* ptr2 = early_alloc(128, 4096);
    printf("ptr2: %p\n", ptr2);
    assert(ptr2 != NULL);
    assert((uintptr_t)ptr2 % 4096 == 0);

    printf("getting current ptr\n");
    phys_addr_t cur = early_alloc_get_current_ptr();
    assert(cur >= (phys_addr_t)(uintptr_t)ptr2 + 128);

    printf("Test passed\n");
    free(_end_mock);
    return 0;
}
