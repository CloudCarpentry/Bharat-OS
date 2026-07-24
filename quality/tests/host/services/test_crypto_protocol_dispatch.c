#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../../core/services/security/crypto/crypto_service.h"

static void test_crypto_protocol_validate(void) {
    printf("Running test_crypto_protocol_validate...\n");

    crypto_request_msg_t req;
    memset(&req, 0, sizeof(req));
    req.header.version = CRYPTO_UAPI_VERSION;
    req.header.opcode = CRYPTO_OP_GET_RANDOM;
    uint32_t valid_len = sizeof(crypto_msg_header_t) + sizeof(crypto_req_get_random_t);

    // 1. NULL pointer
    assert(crypto_protocol_validate(NULL, valid_len) == CRYPTO_STATUS_ERR_BAD_PARAM);

    // 2. Short length
    assert(crypto_protocol_validate(&req, sizeof(crypto_msg_header_t) - 1) == CRYPTO_STATUS_ERR_BAD_PARAM);

    // 3. Wrong version
    req.header.version = CRYPTO_UAPI_VERSION + 1;
    assert(crypto_protocol_validate(&req, valid_len) == CRYPTO_STATUS_ERR_BAD_PARAM);
    req.header.version = CRYPTO_UAPI_VERSION; // Restore

    // 4. Invalid opcode (too low)
    req.header.opcode = CRYPTO_OP_INVALID;
    assert(crypto_protocol_validate(&req, valid_len) == CRYPTO_STATUS_ERR_BAD_PARAM);

    // 5. Invalid opcode (too high)
    req.header.opcode = CRYPTO_OP_ENCRYPT_DUMP + 1;
    assert(crypto_protocol_validate(&req, valid_len) == CRYPTO_STATUS_ERR_BAD_PARAM);

    // 6. Valid header
    req.header.opcode = CRYPTO_OP_GET_RANDOM;
    assert(crypto_protocol_validate(&req, valid_len) == CRYPTO_STATUS_OK);

    printf("test_crypto_protocol_validate passed.\n");
}

static void test_crypto_dispatch_request(void) {
    printf("Running test_crypto_dispatch_request...\n");

    crypto_request_msg_t req;
    crypto_resp_common_t resp;
    memset(&req, 0, sizeof(req));
    memset(&resp, 0, sizeof(resp));

    req.header.version = CRYPTO_UAPI_VERSION;

    // 1. NULL pointers
    assert(crypto_dispatch_request(NULL, &resp) == CRYPTO_STATUS_ERR_BAD_PARAM);
    assert(crypto_dispatch_request(&req, NULL) == CRYPTO_STATUS_ERR_BAD_PARAM);
    assert(crypto_dispatch_request(NULL, NULL) == CRYPTO_STATUS_ERR_BAD_PARAM);

    // 2. Each known opcode returns the expected current phase behavior (NOT_IMPL for all currently)
    crypto_opcode_t known_ops[] = {
        CRYPTO_OP_GET_RANDOM,
        CRYPTO_OP_HASH_INIT,
        CRYPTO_OP_HASH_UPDATE,
        CRYPTO_OP_HASH_FINAL,
        CRYPTO_OP_AEAD_SEAL,
        CRYPTO_OP_AEAD_OPEN,
        CRYPTO_OP_SIGN,
        CRYPTO_OP_VERIFY,
        CRYPTO_OP_KEY_CREATE,
        CRYPTO_OP_KEY_DESTROY,
        CRYPTO_OP_ENCRYPT_DUMP
    };

    for (size_t i = 0; i < sizeof(known_ops) / sizeof(known_ops[0]); i++) {
        req.header.opcode = known_ops[i];
        assert(crypto_dispatch_request(&req, &resp) == CRYPTO_STATUS_ERR_NOT_IMPL);
    }

    // 3. Unknown opcode returns CRYPTO_STATUS_ERR_NOT_IMPL
    req.header.opcode = CRYPTO_OP_ENCRYPT_DUMP + 1;
    assert(crypto_dispatch_request(&req, &resp) == CRYPTO_STATUS_ERR_NOT_IMPL);

    printf("test_crypto_dispatch_request passed.\n");
}

int main(void) {
    printf("Starting Crypto Protocol and Dispatch tests...\n");

    test_crypto_protocol_validate();
    test_crypto_dispatch_request();

    printf("All tests passed.\n");
    return 0;
}
