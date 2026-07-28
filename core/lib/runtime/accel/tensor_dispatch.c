#include "tensor_backend.h"
#include <bharat/accel/accel.h>
#include <string.h>

// Extern prototype for accessing the emulated virtual NPU device driver
extern bharat_accel_device_t* get_virt_accel_mock_device(void);

static tensor_dispatch_stats_t g_stats = {0};
static tensor_dispatch_result_t g_last_result = {NULL, (backend_type_t)0, 0};

// Software fallback implementation
static int sw_process_tensor(tensor_op_t *op) {
    if (!op || !op->input_tensor || !op->output_tensor) return -1;

    // Very dummy SW ReLU logic
    if (op->op == TENSOR_OP_RELU) {
        for (size_t i = 0; i < op->in_elements && i < op->out_elements; i++) {
            op->output_tensor[i] = op->input_tensor[i] > 0 ? op->input_tensor[i] : 0;
        }
    }

    return 0;
}

static backend_interface_t sw_interface = {
    .process_tensor = sw_process_tensor
};

static int sw_init(void) {
    return 0; // Always succeeds
}

static bool sw_is_available(const bharat_hw_caps_t *caps, const backend_dispatch_context_t *ctx) {
    (void)caps;
    (void)ctx;
    return true; // Software fallback is always available
}

static backend_interface_t* sw_get_interface(void) {
    return &sw_interface;
}

static const backend_provider_t sw_tensor_provider = {
    .name = "tensor_sw_fallback",
    .feature_class = CLASS_TENSOR_ML,
    .type = BACKEND_TYPE_SOFTWARE_FALLBACK,
    .priority = 10,
    .init = sw_init,
    .is_available = sw_is_available,
    .get_interface = sw_get_interface
};

// Generic Hardware implementation (mapping to virtual NPU driver)
static int hw_process_tensor(tensor_op_t *op) {
    if (!op || !op->input_tensor || !op->output_tensor) return -1;

    // Fetch actual emulated device
    bharat_accel_device_t *dev = get_virt_accel_mock_device();
    if (!dev || !dev->ops || !dev->ops->submit_npu_job) {
        return -1;
    }

    // Translate tensor operation to a structured virtual accelerator job descriptor
    virt_accel_job_t job;
    memset(&job, 0, sizeof(job));

    if (op->op == TENSOR_OP_RELU) {
        job.opcode = VIRT_ACCEL_OP_RELU_F32;
    } else {
        return -1; // Or K_ERR_UNSUPPORTED
    }

    job.input = op->input_tensor;
    job.input_elements = op->in_elements;
    job.output = op->output_tensor;
    job.output_elements = op->out_elements;

    // Invoke the device driver truthfully
    return dev->ops->submit_npu_job(dev, &job);
}

static backend_interface_t hw_interface = {
    .process_tensor = hw_process_tensor
};

static int hw_init(void) {
    return 0;
}

static bool hw_is_available(const bharat_hw_caps_t *caps, const backend_dispatch_context_t *ctx) {
    if (!caps || !ctx) return false;

    // If strict safe mode is on, bypass hardware
    if (ctx->safe_mode) return false;

    // Only available if the capability flag is PRESENT or REQUIRED
    return (caps->soc.npu == HW_CAP_STATE_PRESENT || caps->soc.npu == HW_CAP_STATE_REQUIRED);
}

static backend_interface_t* hw_get_interface(void) {
    return &hw_interface;
}

static const backend_provider_t hw_tensor_provider = {
    .name = "tensor_hw_generic",
    .feature_class = CLASS_TENSOR_ML,
    .type = BACKEND_TYPE_GENERIC_HARDWARE,
    .priority = 50,
    .init = hw_init,
    .is_available = hw_is_available,
    .get_interface = hw_get_interface
};


int tensor_dispatch_init(void) {
    // Reset registry and clear stats to produce a clean state for isolated tests
    backend_registry_reset();
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_last_result, 0, sizeof(g_last_result));

    int ret = backend_registry_add(&sw_tensor_provider);
    if (ret != 0) return ret;

    return backend_registry_add(&hw_tensor_provider);
}

const backend_provider_t *tensor_select_backend(
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx)
{
    return backend_dispatch_select(CLASS_TENSOR_ML, caps, ctx);
}

int tensor_process(
    tensor_op_t *op,
    const bharat_hw_caps_t *caps,
    const backend_dispatch_context_t *ctx)
{
    if (!op || !caps || !ctx) {
        g_stats.jobs_failed++;
        return -1;
    }

    // Select the optimal backend provider based on discovery and context
    const backend_provider_t *provider = tensor_select_backend(caps, ctx);
    if (!provider) {
        g_stats.jobs_failed++;
        return -1;
    }

    // Populate routing outcomes & last result metadata
    g_last_result.backend_name = provider->name;
    g_last_result.backend_type = provider->type;

    if (provider->type == BACKEND_TYPE_SOFTWARE_FALLBACK) {
        g_stats.backend_sw_fallback++;
        if (ctx->safe_mode) {
            g_stats.safe_mode_fallbacks++;
        } else if (caps->soc.npu != HW_CAP_STATE_PRESENT && caps->soc.npu != HW_CAP_STATE_REQUIRED) {
            g_stats.unavailable_fallbacks++;
        }
    } else {
        g_stats.backend_hw_selected++;
    }

    if (!provider->get_interface) {
        g_last_result.execution_status = -1;
        g_stats.jobs_failed++;
        return -1;
    }

    backend_interface_t *iface = provider->get_interface();
    if (!iface || !iface->process_tensor) {
        g_last_result.execution_status = -1;
        g_stats.jobs_failed++;
        return -1;
    }

    // Truthfully execute
    int status = iface->process_tensor(op);
    g_last_result.execution_status = status;

    if (status == 0) {
        g_stats.jobs_completed++;
    } else {
        g_stats.jobs_failed++;
    }

    return status;
}

void tensor_dispatch_get_stats(tensor_dispatch_stats_t *out) {
    if (out) {
        *out = g_stats;
    }
}

void tensor_dispatch_get_last_result(tensor_dispatch_result_t *out) {
    if (out) {
        *out = g_last_result;
    }
}

void tensor_dispatch_reset_stats(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_last_result, 0, sizeof(g_last_result));
}
