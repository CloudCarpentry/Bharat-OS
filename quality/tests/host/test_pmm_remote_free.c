#include <mm/pmm_pcache.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mocks
uint32_t active_numa_nodes = 1;
uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_disable_interrupts(void) {}
void hal_cpu_enable_interrupts(void) {}
uint32_t g_free_calls = 0;
void mm_free_page(phys_addr_t page_addr) {
    g_free_calls++;
}

void test_pmm_remote_free_drain(void) {
    printf("Running test_pmm_remote_free_drain...\n");
    pmm_pcache_init_all();
    pmm_core_local_init(0);

    // Enqueue some pages
    pmm_pcache_remote_free_enqueue(0, 0x1000);
    pmm_pcache_remote_free_enqueue(0, 0x2000);
    pmm_pcache_remote_free_enqueue(0, 0x3000);

    assert(g_pmm_cores[0].inbox.enqueue_count == 3);

    g_free_calls = 0;
    pmm_drain_remote_frees(0);

    assert(g_free_calls == 3);
    assert(g_pmm_cores[0].inbox.drain_pages == 3);
    assert(g_pmm_cores[0].inbox.drain_batches == 1);
}

int main(void) {
    test_pmm_remote_free_drain();
    printf("All pmm_remote_free host tests passed!\n");
    return 0;
}
