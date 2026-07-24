#include <mm/pmm_pcache.h>
#include <kernel/status.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mock globals needed by pmm_pcache.c
uint32_t active_numa_nodes = 1;

void mm_free_page(phys_addr_t page_addr) {
    (void)page_addr;
}

uint32_t hal_cpu_get_id(void) { return 0; }
void hal_cpu_disable_interrupts(void) {}
void hal_cpu_enable_interrupts(void) {}

page_t *phys_to_page(phys_addr_t phys) {
    static page_t mock_pages[100];
    if (phys >= 0x1000 && phys < 0x1000 + 100*0x1000) {
        return &mock_pages[(phys - 0x1000) / 0x1000];
    }
    return NULL;
}

void test_pmm_pcache_bounds(void) {
    printf("Running test_pmm_pcache_bounds...\n");
    pmm_pcache_init_all();

    assert(pmm_core_id_valid(0) == true);
    assert(pmm_core_id_valid(BHARAT_MAX_CPUS - 1) == true);
    assert(pmm_core_id_valid(BHARAT_MAX_CPUS) == false);
}

void test_pmm_pcache_invariants(void) {
    printf("Running test_pmm_pcache_invariants...\n");
    page_t p;

    // Test pmm_page_can_enter_pcache
    p.pin_count = 0;
    assert(pmm_page_can_enter_pcache(&p) == true);
    p.pin_count = 1;
    assert(pmm_page_can_enter_pcache(&p) == false);

    // Test pmm_page_owned_by_core
    p.owner_core_id = 5;
    assert(pmm_page_owned_by_core(&p, 5) == true);
    assert(pmm_page_owned_by_core(&p, 0) == false);
}

void test_pmm_negative_cases(void) {
    printf("Running test_pmm_negative_cases...\n");
    pmm_pcache_init_all();

    // Case 1: remote-free to invalid CPU
    assert(pmm_pcache_remote_free_enqueue(BHARAT_MAX_CPUS, 0x1000) == K_ERR_INVALID_ARG);

    // Case 2: remote-free to offline CPU
    assert(pmm_pcache_remote_free_enqueue(1, 0x1000) == K_ERR_BAD_STATE);

    // Case 3: double free check (this would typically be in mm_free_page,
    // but we can test pmm_page_can_enter_pcache here)
    page_t p;
    p.pin_count = 1;
    assert(pmm_page_can_enter_pcache(&p) == false);
}

void test_pmm_remote_free(void) {
    printf("Running test_pmm_remote_free...\n");
    pmm_pcache_init_all();

    // Core not active yet
    assert(pmm_pcache_remote_free_enqueue(0, 0x1000) == K_ERR_BAD_STATE);

    pmm_core_local_init(0);
    assert(pmm_pcache_remote_free_enqueue(0, 0x1000) == K_OK);

    assert(g_pmm_cores[0].inbox.enqueue_count == 1);
    assert(g_pmm_cores[0].inbox.pages[0] == 0x1000);

    // Test inbox overflow
    for (int i = 0; i < PMM_INBOX_SIZE - 2; i++) {
        assert(pmm_pcache_remote_free_enqueue(0, 0x2000 + i*0x1000) == K_OK);
    }
    // Now it should be full (PMM_INBOX_SIZE-1 entries because of head/tail circular logic)
    assert(pmm_pcache_remote_free_enqueue(0, 0xFFFF) == K_ERR_NO_MEMORY);
    assert(g_pmm_cores[0].inbox.enqueue_failures == 1);
}

int main(void) {
    test_pmm_pcache_bounds();
    test_pmm_pcache_invariants();
    test_pmm_negative_cases();
    test_pmm_remote_free();
    printf("All pmm_pcache host tests passed!\n");
    return 0;
}
