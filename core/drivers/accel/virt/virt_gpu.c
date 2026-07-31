#include "virt_accel_test_hooks.h"
#include <kernel/status.h>
#include <string.h>

static uint64_t g_virt_gpu_submit_count = 0;
static bool g_virt_gpu_fail_injection = false;

uint64_t virt_gpu_get_submit_count(void) {
    return g_virt_gpu_submit_count;
}

void virt_gpu_reset_submit_count(void) {
    g_virt_gpu_submit_count = 0;
}

void virt_gpu_set_fail_injection(bool enable) {
    g_virt_gpu_fail_injection = enable;
}

static int virt_gpu_init(struct bharat_accel_device *dev) {
    (void)dev;
    return K_OK;
}

static void virt_gpu_deinit(struct bharat_accel_device *dev) {
    (void)dev;
}

static int virt_gpu_submit_job(struct bharat_accel_device *dev, void *job_descriptor) {
    if (!dev) return -1; // K_ERR_INVALID_ARG or -1

    g_virt_gpu_submit_count++;

    if (g_virt_gpu_fail_injection) {
        return -2; // Simulated hardware failure
    }

    if (!job_descriptor) {
        return -1;
    }

    virt_gpu_job_t *job = (virt_gpu_job_t *)job_descriptor;

    if (job->opcode == VIRT_ACCEL_OP_GPU_ADD_ONE_F32) {
        if (!job->input || !job->output || job->input_elements == 0) {
            return -1;
        }
        if (job->output_elements < job->input_elements) {
            return -4; // Overflow
        }
        const float *in = (const float *)job->input;
        float *out = (float *)job->output;
        for (size_t i = 0; i < job->input_elements; i++) {
            out[i] = in[i] + 1.0f;
        }
        return K_OK;
    } else if (job->opcode == VIRT_ACCEL_OP_GPU_ADD_ONE_INT8) {
        if (!job->input || !job->output || job->input_elements == 0) {
            return -1;
        }
        if (job->output_elements < job->input_elements) {
            return -4; // Overflow
        }
        const int8_t *in = (const int8_t *)job->input;
        int8_t *out = (int8_t *)job->output;
        for (size_t i = 0; i < job->input_elements; i++) {
            int32_t val = (int32_t)in[i] + 1;
            if (val > 127) val = 127;
            if (val < -128) val = -128;
            out[i] = (int8_t)val;
        }
        return K_OK;
    }

    return -3; // Unsupported opcode
}

static const bharat_accel_device_ops_t virt_gpu_ops = {
    .init = virt_gpu_init,
    .deinit = virt_gpu_deinit,
    .dma_memcpy = NULL,
    .blit = NULL,
    .fill = NULL,
    .submit_gpu_job = virt_gpu_submit_job,
};

static bharat_accel_device_t virt_gpu_dev = {
    .name = "virt_gpu_0",
    .id = 1,
    .capabilities = BHARAT_ACCEL_CAP_GPU | BHARAT_ACCEL_CAP_DMA,
    .ops = &virt_gpu_ops,
    .priv_data = NULL,
};

bharat_accel_device_t* get_virt_gpu_mock_device(void) {
    return &virt_gpu_dev;
}
