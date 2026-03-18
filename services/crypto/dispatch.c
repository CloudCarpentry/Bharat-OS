#include "crypto_service.h"

crypto_status_t crypto_dispatch_request(const crypto_request_msg_t *req, crypto_resp_common_t *resp) {
    switch (req->header.opcode) {
        case CRYPTO_OP_GET_RANDOM:
            return handle_get_random(&req->payload.get_random, resp);
        case CRYPTO_OP_HASH_INIT:
            return handle_hash_init(&req->payload.hash, resp);
        case CRYPTO_OP_HASH_UPDATE:
            return handle_hash_update(&req->payload.hash, resp);
        case CRYPTO_OP_HASH_FINAL:
            return handle_hash_final(&req->payload.hash, resp);
        case CRYPTO_OP_AEAD_SEAL:
            return handle_aead_seal(&req->payload.aead, resp);
        case CRYPTO_OP_AEAD_OPEN:
            return handle_aead_open(&req->payload.aead, resp);
        case CRYPTO_OP_SIGN:
            return handle_sign(&req->payload.sign, resp);
        case CRYPTO_OP_VERIFY:
            return handle_verify(&req->payload.verify, resp);
        case CRYPTO_OP_KEY_CREATE:
            return handle_key_create(&req->payload.key_create, resp);
        case CRYPTO_OP_KEY_DESTROY:
            return handle_key_destroy(&req->payload.key_destroy, resp);
        case CRYPTO_OP_ENCRYPT_DUMP:
            return handle_encrypt_dump(&req->payload.encrypt_dump, resp);
        default:
            return CRYPTO_STATUS_ERR_NOT_IMPL;
    }
}

// Handlers default to NOT_IMPLEMENTED for this phase
crypto_status_t handle_get_random(const crypto_req_get_random_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_hash_init(const crypto_req_hash_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_hash_update(const crypto_req_hash_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_hash_final(const crypto_req_hash_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_aead_seal(const crypto_req_aead_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_aead_open(const crypto_req_aead_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_sign(const crypto_req_sign_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_verify(const crypto_req_verify_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_key_create(const crypto_req_key_create_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_key_destroy(const crypto_req_key_destroy_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}

crypto_status_t handle_encrypt_dump(const crypto_req_encrypt_dump_t *req, crypto_resp_common_t *resp) {
    (void)req; (void)resp;
    return CRYPTO_STATUS_ERR_NOT_IMPL;
}
