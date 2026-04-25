#include "crypto_backend.h"
#include "../../../arch/arch_crypto.h"

static int sw_process_op(crypto_op_t *op) {
    if (!op || !op->in || !op->out) return -1;

    for (size_t i = 0; i < op->in_len && i < op->out_len; i++) {
        op->out[i] = op->in[i] ^ 0xAA;
    }

    return 0;
}

static backend_interface_t sw_interface = {
    .process_op = sw_process_op
};

static int sw_init(void) {
    return 0;
}

static bool sw_is_available(const bharat_hw_caps_t *caps, const backend_dispatch_context_t *ctx) {
    (void)caps;
    (void)ctx;
    return true;
}

static backend_interface_t* sw_get_interface(void) {
    return &sw_interface;
}

static const backend_provider_t sw_crypto_provider = {
    .name = "crypto_sw_fallback",
    .feature_class = CLASS_CRYPTO,
    .type = BACKEND_TYPE_SOFTWARE_FALLBACK,
    .priority = 10,
    .init = sw_init,
    .is_available = sw_is_available,
    .get_interface = sw_get_interface
};

static int hw_process_op(crypto_op_t *op) {
    if (!op || !op->in || !op->out) return -1;

    // First production accelerated helper path (runtime-gated):
    // - AES-NI present => lightweight xor/rotate transform placeholder hook.
    // - PCLMUL present => lightweight GF-like mix placeholder hook.
    const bool has_aes = arch_crypto_has_aes();
    const bool has_pclmul = arch_crypto_has_poly_mul();

    if (!arch_crypto_accel_available() || (!has_aes && !has_pclmul)) {
        return sw_process_op(op);
    }

    for (size_t i = 0; i < op->in_len && i < op->out_len; i++) {
        uint8_t v = op->in[i];
        if (has_aes) {
            v = (uint8_t)((v << 1) | (v >> 7));
            v ^= 0x63u;
        }
        if (has_pclmul) {
            v ^= (uint8_t)(i * 0x1Du);
        }
        op->out[i] = v;
    }

    return 0;
}

static backend_interface_t hw_interface = {
    .process_op = hw_process_op
};

static int hw_init(void) {
    return 0;
}

static bool hw_is_available(const bharat_hw_caps_t *caps, const backend_dispatch_context_t *ctx) {
    if (!caps || !ctx) return false;
    if (ctx->safe_mode) return false;

    if (caps->cpu.crypto == HW_CAP_STATE_PRESENT || caps->cpu.crypto == HW_CAP_STATE_REQUIRED) {
        return arch_crypto_accel_available();
    }

    return false;
}

static backend_interface_t* hw_get_interface(void) {
    return &hw_interface;
}

static const backend_provider_t hw_crypto_provider = {
    .name = "crypto_hw_generic",
    .feature_class = CLASS_CRYPTO,
    .type = BACKEND_TYPE_GENERIC_HARDWARE,
    .priority = 50,
    .init = hw_init,
    .is_available = hw_is_available,
    .get_interface = hw_get_interface
};

int crypto_dispatch_init(void) {
    int ret = backend_registry_add(&sw_crypto_provider);
    if (ret != 0) return ret;

    return backend_registry_add(&hw_crypto_provider);
}

int crypto_process(crypto_op_t *op, const backend_dispatch_context_t *ctx) {
    if (!op || !ctx) return -1;

    bharat_hw_caps_t system_caps = {0};
    system_caps.cpu.crypto = HW_CAP_STATE_PRESENT;

    const backend_provider_t *provider = backend_dispatch_select(CLASS_CRYPTO, &system_caps, ctx);

    if (!provider || !provider->get_interface) return -1;

    backend_interface_t *iface = provider->get_interface();
    if (!iface || !iface->process_op) return -1;

    return iface->process_op(op);
}
