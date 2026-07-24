#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../kernel/include/mm_zswap.h"
#include "../kernel/include/benchmark/benchmark.h"

#define NUM_PAGES 1000
#define LOOKUP_ITERATIONS 10000

// We need an array to hold the allocated pages to simulate physical addresses
static phys_addr_t g_test_pages[NUM_PAGES];

int main(void) {
    printf("Initializing ZSwap benchmark...\n");

    int res = zswap_init();
    if (res != 0) {
        printf("Failed to init zswap. Exiting.\n");
        return -1;
    }

    // Populate zswap pool
    for (int i = 0; i < NUM_PAGES; i++) {
        // We will just allocate memory to represent the page to read from.
        uint8_t* p = malloc(4096);
        for(int j=0; j<4096; j++) {
            p[j] = 0; // Extremely compressible data
        }

        // Update g_test_pages to be valid host virtual addresses
        g_test_pages[i] = (phys_addr_t)(uintptr_t)p;

        // Store it
        int stored = zswap_store_page(g_test_pages[i]);
        if (stored != 0) {
            printf("Failed to store page %d\n", i);
        }
    }

    benchmark_ctx_t ctx;
    benchmark_start(&ctx, "ZSwap Load Page (Random Lookup)", BENCHMARK_LEVEL_0_REF, LOOKUP_ITERATIONS);

    unsigned int seed = 12345;
    int success_count = 0;

    for (int i = 0; i < LOOKUP_ITERATIONS; i++) {
        int idx = rand_r(&seed) % NUM_PAGES;
        phys_addr_t page_to_load = g_test_pages[idx];

        // Load the page
        if (zswap_load_page(page_to_load) == 0) {
            success_count++;
            // Re-store it to keep the pool populated
            zswap_store_page(page_to_load);
        }
    }

    benchmark_stop(&ctx);

    benchmark_result_t result;
    benchmark_record(&ctx, &result);
    benchmark_print(&result, "ZSwap Load Page Lookup");

    printf("Successful lookups: %d / %d\n", success_count, LOOKUP_ITERATIONS);

    // Cleanup
    for (int i = 0; i < NUM_PAGES; i++) {
        free((void*)(uintptr_t)g_test_pages[i]);
    }

    return 0;
}
