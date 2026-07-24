#include "tensor_backend.h"

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

// Generic Hardware implementation (e.g., discrete NPU/GPU hooks)
static int hw_process_tensor(tensor_op_t *op) {
    if (!op || !op->input_tensor || !op->output_tensor) return -1;
    // Just a hook for HW logic via accelmgr or raw MMIO
    return 0;
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
    int ret = backend_registry_add(&sw_tensor_provider);
    if (ret != 0) return ret;

    return backend_registry_add(&hw_tensor_provider);
}

int tensor_process(tensor_op_t *op, const backend_dispatch_context_t *ctx) {
    if (!op || !ctx) return -1;

    bharat_hw_caps_t system_caps;
    system_caps.soc.npu = HW_CAP_STATE_PRESENT;

    const backend_provider_t *provider = backend_dispatch_select(CLASS_TENSOR_ML, &system_caps, ctx);

    if (!provider || !provider->get_interface) return -1;

    backend_interface_t *iface = provider->get_interface();
    if (!iface || !iface->process_tensor) return -1;

    return iface->process_tensor(op);
}
