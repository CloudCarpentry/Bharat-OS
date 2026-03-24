#include <stdint.h>
#include <string.h>

#include <bharat/uapi/sys_errno.h>
#include <bharat/uapi/crypto/contract.h>
#include <security/crypto_caps.h>
#include <security/crypto_contract.h>
#include <security/crypto_registry.h>
#include <tests/ktest.h>

/* Mock memory allocator stubs for tests */
#include <stdlib.h>
void *kmalloc(size_t size) { return malloc(size); }
void kfree(void *ptr) { free(ptr); }

/* Mock provider implementation */
static int mock_invoke(crypto_op_args_t *args) {
    if (args->op == CRYPTO_OP_HASH_BUFFER) {
        if (args->output_buf && args->output_len > 0) {
            memset(args->output_buf, 0x42, args->output_len);
            return 0;
        }
    }
    return -SYS_ENOSYS;
}

static void mock_zeroize(void *buf, size_t len) {
    if (buf) memset(buf, 0x00, len);
}

static int mock_get_random(void *buf, size_t len) {
    if (buf) {
        memset(buf, 0x11, len);
        return 0;
    }
    return -SYS_EINVAL;
}

static const crypto_provider_ops_t mock_ops = {
    .invoke = mock_invoke,
    .zeroize = mock_zeroize,
    .get_random = mock_get_random,
};

static crypto_provider_info_t mock_info = {
    .backend_class = CRYPTO_BACKEND_CPU_ACCEL,
    .supported_ops_mask = (1 << CRYPTO_OP_HASH_BUFFER) | (1 << CRYPTO_OP_GET_RANDOM),
    .name = "mock_accel",
};

/* --- Tests --- */

static int test_provider_registration(void) {
    crypto_registry_init();

    int provider_id = crypto_provider_register(&mock_info, &mock_ops);
    if (provider_id <= 0) return -1;

    const crypto_provider_node_t *node = crypto_provider_find(provider_id);
    if (node == NULL) return -2;
    if (node->id != provider_id) return -3;
    if (strcmp(node->info.name, "mock_accel") != 0) return -4;

    int err = crypto_provider_unregister(provider_id);
    if (err != 0) return -5;

    node = crypto_provider_find(provider_id);
    if (node != NULL) return -6;

    return 0;
}

static int test_capability_checks(void) {
    crypto_registry_init();

    int provider_id = crypto_provider_register(&mock_info, &mock_ops);
    if (provider_id <= 0) return -1;

    crypto_op_args_t args = {
        .op = CRYPTO_OP_HASH_BUFFER,
        .input_buf = NULL,
        .input_len = 0,
        .output_buf = NULL,
        .output_len = 0,
    };

    /* 1. Try without capability -> Should fail with EPERM */
    crypto_cap_revoke(CAP_TYPE_CRYPTO_KEY);
    int err = crypto_provider_invoke(provider_id, &args);
    if (err != -SYS_EPERM) return -2;

    /* 2. Grant capability -> Should pass cap check but fail args check or return success depending on invoke logic */
    crypto_cap_grant(CAP_TYPE_CRYPTO_KEY);

    uint8_t out_buf[16];
    args.output_buf = out_buf;
    args.output_len = sizeof(out_buf);

    err = crypto_provider_invoke(provider_id, &args);
    if (err != 0) return -3;
    if (out_buf[0] != 0x42) return -4;

    crypto_provider_unregister(provider_id);

    return 0;
}

static int test_unsupported_operation(void) {
    crypto_registry_init();

    int provider_id = crypto_provider_register(&mock_info, &mock_ops);
    if (provider_id <= 0) return -1;

    crypto_op_args_t args = {
        .op = CRYPTO_OP_SEAL, /* Mock provider doesn't support SEAL */
    };

    crypto_cap_grant(CAP_TYPE_SEALER);

    int err = crypto_provider_invoke(provider_id, &args);
    if (err != -SYS_ENOSYS) return -2;

    crypto_provider_unregister(provider_id);
    return 0;
}

static int test_get_random_convenience(void) {
    crypto_registry_init();

    crypto_provider_info_t rng_info = {
        .backend_class = CRYPTO_BACKEND_RNG_PROVIDER,
        .supported_ops_mask = (1 << CRYPTO_OP_GET_RANDOM),
        .name = "mock_rng",
    };

    int provider_id = crypto_provider_register(&rng_info, &mock_ops);
    if (provider_id <= 0) return -1;

    uint8_t buf[8] = {0};

    /* Without CAP_TYPE_RNG */
    crypto_cap_revoke(CAP_TYPE_RNG);
    int err = crypto_get_random(buf, sizeof(buf));
    if (err != -SYS_EPERM) return -2;

    /* With CAP_TYPE_RNG */
    crypto_cap_grant(CAP_TYPE_RNG);
    err = crypto_get_random(buf, sizeof(buf));
    if (err != 0) return -3;
    if (buf[0] != 0x11) return -4;

    crypto_provider_unregister(provider_id);
    return 0;
}

static int ktest_crypto_registry_run(void) {
    if (test_provider_registration() != 0) return -1;
    if (test_capability_checks() != 0) return -1;
    if (test_unsupported_operation() != 0) return -1;
    if (test_get_random_convenience() != 0) return -1;
    return 0;
}

REGISTER_BOOT_SELFTEST("crypto_registry", "security", ktest_crypto_registry_run, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
