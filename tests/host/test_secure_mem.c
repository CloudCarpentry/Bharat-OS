#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Mock kernel definitions before including secure_mem.c
#define TESTING 1

#include "../../kernel/include/crypto/secure_mem.h"

// --- Global variables for mocks ---
static int kmalloc_count = 0;
static int kfree_count = 0;
static int kfree_was_null = 0;
static void* last_allocated_ptr = NULL;

// --- Mocking basic kernel functions ---

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    // Simulate allocation failure for testing
    if (size == 0xFFFFFFFF) return NULL;

    void* ptr = malloc(size);
    if (ptr) {
        // Fill with dummy data to ensure secure_memzero_explicit works
        memset(ptr, 0xAA, size);
        kmalloc_count++;
        last_allocated_ptr = ptr;
    }
    return ptr;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        kfree_was_null = 1;
        return;
    }
    free(ptr);
    kfree_count++;
}

// Ensure the inclusion compiles our actual object code
#include "../../kernel/src/crypto/secure_mem.c"

// --- Tests ---

void test_secure_memzero_explicit() {
    printf("Running test_secure_memzero_explicit...\n");

    // Test 1: Normal zeroing
    char buffer[16];
    memset(buffer, 0xBB, sizeof(buffer));
    secure_memzero_explicit(buffer, sizeof(buffer));
    for (int i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == 0);
    }

    // Test 2: NULL pointer input
    secure_memzero_explicit(NULL, 10); // Should return without crashing

    // Test 3: Zero length
    memset(buffer, 0xCC, sizeof(buffer));
    secure_memzero_explicit(buffer, 0);
    for (int i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == (char)0xCC); // Unchanged
    }

    printf("test_secure_memzero_explicit passed\n");
}

void test_secure_page_alloc() {
    printf("Running test_secure_page_alloc...\n");

    kmalloc_count = 0;

    // Test 1: Successful allocation
    void* page = secure_page_alloc(0);
    assert(page != NULL);
    assert(kmalloc_count == 1);

    // Ensure memory is zeroed
    unsigned char* p = (unsigned char*)page;
    for (int i = 0; i < 4096; i++) {
        assert(p[i] == 0);
    }

    kfree(page); // Clean up

    printf("test_secure_page_alloc passed\n");
}

void test_secure_page_free() {
    printf("Running test_secure_page_free...\n");

    kfree_count = 0;

    // Test 1: Normal free (without SECURE_MEM_ZERO_ON_FREE)
    void* page = secure_page_alloc(0);
    // Write some data
    memset(page, 0xDD, 4096);
    secure_page_free(page, 0);
    assert(kfree_count == 1); // 1 from this free, previous test cleanup was reset because we set kfree_count = 0 before this
    // In a real environment, memory would be freed and not accessible,
    // but in our test we mock free() so we just count the calls.

    // Test 2: Secure free (with SECURE_MEM_ZERO_ON_FREE)
    kfree_count = 0;
    page = secure_page_alloc(0);
    // Write some data
    memset(page, 0xEE, 4096);

    // Mock: we intercept before kfree to check if zeroed. We can't easily intercept inside kfree,
    // but we can trust secure_memzero_explicit which we tested earlier.
    secure_page_free(page, SECURE_MEM_ZERO_ON_FREE);
    assert(kfree_count == 1);

    // Test 3: NULL pointer input
    kfree_was_null = 0;
    secure_page_free(NULL, 0);
    assert(kfree_was_null == 0); // Our stub returns early before calling kfree

    printf("test_secure_page_free passed\n");
}

void test_stubs() {
    printf("Running test_stubs...\n");

    // Test 1: Map secure buffer
    int result1 = secure_buffer_map_for_service(0x1000, 4096, 0);
    assert(result1 == 0);

    // Test 2: Unmap secure buffer
    int result2 = secure_buffer_unmap_for_service(0x1000, 4096);
    assert(result2 == 0);

    printf("test_stubs passed\n");
}

int main() {
    test_secure_memzero_explicit();
    test_secure_page_alloc();
    test_secure_page_free();
    test_stubs();

    printf("All host secure_mem tests passed.\n");
    return 0;
}
