#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mm.h"
#include "security/isolation.h"
#include "security/vfio.h"
#include "hal/iommu.h"

struct hal_iommu_domain { int dummy; };
struct hal_iommu_group { int dummy; };
struct bharat_device { int dummy; };

int hal_iommu_init(void) { return 0; }
int hal_iommu_domain_create(const bharat_iommu_domain_config_t *cfg, hal_iommu_domain_t **out) {
    static struct hal_iommu_domain dummy_domain;
    *out = &dummy_domain;
    return 0;
}
int hal_iommu_domain_destroy(hal_iommu_domain_t *domain) { return 0; }
int hal_iommu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) { return 0; }
int hal_iommu_group_detach(hal_iommu_group_t *group) { return 0; }
int hal_iommu_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) { return 0; }
int hal_iommu_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) { return 0; }
int hal_iommu_block_device(bharat_device_t *dev) { return 0; }
int hal_iommu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    static struct hal_iommu_group dummy_group;
    *out_group = &dummy_group;
    return 0;
}

void test_iommu_lifecycle() {
    printf("Testing IOMMU Lifecycle...\n");

    bharat_isolation_init();

    bharat_iommu_domain_config_t cfg = {
        .type = BHARAT_IOMMU_DOMAIN_KERNEL,
        .flags = 0
    };
    bharat_iommu_domain_t *domain = NULL;

    int ret = bharat_iommu_domain_create(&cfg, &domain);
    assert(ret == 0);
    assert(domain != NULL);

    bharat_iommu_group_t *group = NULL;
    bharat_device_t dev; // dummy
    ret = bharat_iommu_get_group(&dev, &group);
    assert(ret == 0);
    assert(group != NULL);

    ret = bharat_iommu_group_attach(group, domain);
    assert(ret == 0);

    ret = bharat_iommu_map(domain, 0x1000, 0x2000, 4096, 0);
    assert(ret == 0);

    ret = bharat_iommu_unmap(domain, 0x1000, 4096);
    assert(ret == 0);

    ret = bharat_iommu_group_detach(group);
    assert(ret == 0);

    ret = bharat_iommu_domain_destroy(domain);
    assert(ret == 0);
    printf("IOMMU Lifecycle tests passed.\n");
}

void test_vfio_lifecycle() {
    printf("Testing VFIO Lifecycle...\n");

    bharat_vfio_container_t *container = NULL;
    int ret = bharat_vfio_container_create(&container);
    assert(ret == 0);
    assert(container != NULL);
    assert(container->domain == NULL);
    assert(container->group_refcount == 0);

    bharat_iommu_group_t *iommu_group = NULL;
    bharat_device_t dev;
    ret = bharat_iommu_get_group(&dev, &iommu_group);
    assert(ret == 0);

    bharat_vfio_group_t vfio_group;
    vfio_group.group_id = 1;
    vfio_group.flags = 0;
    vfio_group.domain = NULL;
    vfio_group.iommu_group = iommu_group;

    ret = bharat_vfio_group_attach_container(&vfio_group, container);
    assert(ret == 0);
    assert(container->domain != NULL);
    assert(container->group_refcount == 1);

    ret = bharat_vfio_group_detach_container(&vfio_group);
    assert(ret == 0);
    assert(container->domain == NULL);
    assert(container->group_refcount == 0);

    ret = bharat_vfio_container_destroy(container);
    assert(ret == 0);

    printf("VFIO Lifecycle tests passed.\n");
}

int main() {
    test_iommu_lifecycle();
    test_vfio_lifecycle();
    printf("All test_dma_iommu tests passed.\n");
    return 0;
}