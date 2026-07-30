#include <bharat/accel/accel.h>
#include <kernel/status.h>

// Real emulated device driver with test-support diagnostics.

static uint64_t g_virt_accel_submit_count = 0;
static bool g_virt_accel_fail_injection = false;

uint64_t virt_accel_get_submit_count(void) {
    return g_virt_accel_submit_count;
}

void virt_accel_reset_submit_count(void) {
    g_virt_accel_submit_count = 0;
}

void virt_accel_set_fail_injection(bool enable) {
    g_virt_accel_fail_injection = enable;
}

static int virt_accel_init(struct bharat_accel_device *dev) {
    (void)dev;
    return K_OK;
}

static void virt_accel_deinit(struct bharat_accel_device *dev) {
    (void)dev;
}

static int virt_accel_submit_job(struct bharat_accel_device *dev, void *job_descriptor) {
    if (!dev) return K_ERR_INVALID_ARG;

    // Increment submit count on each entry to prove that invocation reached the hardware layer
    g_virt_accel_submit_count++;

    if (g_virt_accel_fail_injection) {
        return K_ERR_DEV_MMIO_FAULT; // Simulated hardware failure
    }

    if (!job_descriptor) {
        return K_ERR_INVALID_ARG;
    }

    virt_accel_job_t *job = (virt_accel_job_t *)job_descriptor;

    if (job->opcode != VIRT_ACCEL_OP_RELU_F32) {
        return K_ERR_UNSUPPORTED;
    }

    if (!job->input || !job->output || job->input_elements == 0) {
        return K_ERR_INVALID_ARG;
    }

    if (job->output_elements < job->input_elements) {
        return K_ERR_OVERFLOW;
    }

    // Perform actual FP32 ReLU element-by-element
    for (size_t i = 0; i < job->input_elements; i++) {
        job->output[i] = job->input[i] > 0.0f ? job->input[i] : 0.0f;
    }

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
