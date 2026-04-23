#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mm/iommu.h"
#include "kernel/status.h"

// We test the stubs exposed by iommu_stub.c when BHARAT_ENABLE_IOMMU is OFF.
// In host tests, this may be compiled standalone.

// Provide mock for console_write_raw used by the stub
void console_write_raw(const char *str, size_t len) {
    // Optional: write to stderr for test debugging if needed
    // fwrite(str, 1, len, stderr);
}

void test_iommu_stub_lifecycle() {
    printf("Testing IOMMU Stub Lifecycle...\n");

    int ret = iommu_init();
    assert(ret == K_OK);

    assert(iommu_available() == false);
    assert(iommu_enabled() == false);

    iommu_domain_t *domain = iommu_domain_create(0);
    assert(domain == NULL);

    // Test mapping which should explicitly return an error
    ret = iommu_map(domain, 0x1000, 0x2000, 4096, 0, 0);
    assert(ret == K_ERR_UNSUPPORTED);

    ret = iommu_unmap(domain, 0x1000, 4096);
    assert(ret == K_ERR_UNSUPPORTED);

    // Create dummy device
    iommu_device_t dev;
    dev.bdf = 0;
    dev.mode = IOMMU_DEV_UNMANAGED;
    dev.hal_priv = NULL;

    ret = iommu_attach_device(domain, &dev);
    assert(ret == K_ERR_UNSUPPORTED);

    ret = iommu_detach_device(&dev);
    assert(ret == K_ERR_UNSUPPORTED);

    ret = iommu_invalidate_domain(domain);
    assert(ret == K_ERR_UNSUPPORTED);

    ret = iommu_invalidate_range(domain, 0x1000, 4096);
    assert(ret == K_ERR_UNSUPPORTED);

    dma_translation_mode_t mode;
    ret = iommu_get_dma_mode(&dev, &mode);
    assert(ret == K_OK);
    assert(mode == DMA_TRANSLATION_DIRECT); // Expected fallback mode

    // cleanup
    iommu_domain_destroy(domain);

    printf("IOMMU Stub Lifecycle tests passed.\n");
}

int main() {
    test_iommu_stub_lifecycle();
    printf("All test_iommu_stub tests passed.\n");
    return 0;
}
