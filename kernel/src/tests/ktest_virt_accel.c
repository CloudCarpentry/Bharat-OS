#include "../../include/bharat/accel/accel.h"
#include "../../include/kernel/status.h"
#include "../../include/mm/accel_mem.h"
#include "../../include/mm/dma.h"
#include <stddef.h>

#define ASSERT_OK(x) do { if ((x) != K_OK) { return -1; } } while(0)
#define ASSERT_TRUE(x) do { if (!(x)) { return -1; } } while(0)

extern bharat_accel_device_t* get_virt_accel_mock_device(void);

int test_virt_accel_smoke(void) {
    bharat_accel_device_t* virt_accel_dev = get_virt_accel_mock_device();

    virt_accel_dev->ops->init(virt_accel_dev);

    accel_buffer_t *buf = NULL;
    int ret = accel_mem_buffer_create(4096, PAGE_SIZE, ACCEL_MEM_SHARED | ACCEL_MEM_STREAMING, (cap_handle_t){0}, &buf);
    ASSERT_OK(ret);

    ret = accel_mem_pin(buf);
    ASSERT_OK(ret);

    // Mock domain
    iova_domain_t mock_domain = {0};

    ret = accel_sg_build(buf);
    ASSERT_OK(ret);

    // Bound explicit state
    buf->state = ACCEL_BUF_STATE_IOVA_BOUND;
    buf->iommu_domain = (struct iommu_domain*)&mock_domain;

    ret = accel_mem_sync(buf, ACCEL_SYNC_TO_DEVICE);
    ASSERT_OK(ret);

    ret = virt_accel_dev->ops->submit_npu_job(virt_accel_dev, NULL);
    ASSERT_OK(ret);

    accel_mem_teardown(buf);
    ASSERT_TRUE(buf->state == ACCEL_BUF_STATE_DESTROYED);

    virt_accel_dev->ops->deinit(virt_accel_dev);

    return 0; // PASS
}
