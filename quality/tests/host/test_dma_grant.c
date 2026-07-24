#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "mm/dma_grant.h"
#include "kernel/status.h"
#include "mm/iommu.h"

// Mocks
static bool g_iommu_avail = false;
#ifndef BHARAT_HOST_TEST_STUB
bool iommu_available(void) { return g_iommu_avail; }
#endif

// Mock for console
void console_write_raw(const char *s, size_t len) { (void)s; (void)len; }

void test_dma_grant_lifecycle(void) {
    printf("Testing DMA grant lifecycle...\n");

    bh_dma_grant_create_args_t args = {0};
    args.paddr = 0x1000;
    args.iova = 0x1000;
    args.length = 0x1000;
    args.owner_id = 1;
    args.device_id = 2;

    bh_dma_grant_id_t grant;
    bh_dma_grant_state_t state;

    // Test creation
    kstatus_t status = bh_dma_grant_create(&args, &grant);
    assert(status == K_OK);
    bh_dma_grant_get_state(grant, &state);
    assert(state == BH_DMA_GRANT_CREATED);

    // Test mapping with IOMMU off (direct allowed)
    g_iommu_avail = false;
    status = bh_dma_grant_map(grant);
    assert(status == K_OK);
    bh_dma_grant_get_state(grant, &state);
    assert(state == BH_DMA_GRANT_MAPPED);

    // Test activation
    status = bh_dma_grant_activate(grant);
    assert(status == K_OK);
    bh_dma_grant_get_state(grant, &state);
    assert(state == BH_DMA_GRANT_ACTIVE);

    // Test revocation
    status = bh_dma_grant_revoke(grant, 0);
    assert(status == K_OK);
    bh_dma_grant_get_state(grant, &state);
    assert(state == BH_DMA_GRANT_REVOKED);

    // Test destruction
    status = bh_dma_grant_destroy(grant);
    assert(status == K_OK);

    // Test mapping with IOMMU off and translation required (should fail)
    args.iova = 0x2000; // Translation required
    status = bh_dma_grant_create(&args, &grant);
    assert(status == K_OK);
    status = bh_dma_grant_map(grant);
    assert(status == K_ERR_UNSUPPORTED);

    printf("DMA grant lifecycle tests passed!\n");
}

int main() {
    test_dma_grant_lifecycle();
    return 0;
}
