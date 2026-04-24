#include "../../include/bharat/accel/accel.h"
#include "../../kernel/include/kernel/status.h"

// Pure mock device driver, strictly backend logic. No test logic here.

static int virt_accel_init(struct bharat_accel_device *dev) {
    (void)dev;
    return K_OK;
}

static void virt_accel_deinit(struct bharat_accel_device *dev) {
    (void)dev;
}

static int virt_accel_submit_job(struct bharat_accel_device *dev, void *job_descriptor) {
    (void)dev;
    (void)job_descriptor;
    return K_OK;
}

static const bharat_accel_device_ops_t virt_accel_ops = {
    .init = virt_accel_init,
    .deinit = virt_accel_deinit,
    .dma_memcpy = NULL,
    .blit = NULL,
    .fill = NULL,
    .submit_npu_job = virt_accel_submit_job,
};

static bharat_accel_device_t virt_accel_dev = {
    .name = "virt_accel_0",
    .id = 0,
    .capabilities = BHARAT_ACCEL_CAP_NPU | BHARAT_ACCEL_CAP_DMA,
    .ops = &virt_accel_ops,
    .priv_data = NULL,
};

// Export a helper to tests to get the driver struct instance
bharat_accel_device_t* get_virt_accel_mock_device(void) {
    return &virt_accel_dev;
}
