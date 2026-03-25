#ifndef CRYPTO_SERVICE_H
#define CRYPTO_SERVICE_H

#include "crypto/crypto_uapi.h"

/* Protocol and validation */
crypto_status_t crypto_protocol_validate(const crypto_request_msg_t *req, uint32_t len);

/* Message dispatch */
crypto_status_t crypto_dispatch_request(const crypto_request_msg_t *req, crypto_resp_common_t *resp);

/* Handlers */
crypto_status_t handle_get_random(const crypto_req_get_random_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_hash_init(const crypto_req_hash_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_hash_update(const crypto_req_hash_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_hash_final(const crypto_req_hash_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_aead_seal(const crypto_req_aead_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_aead_open(const crypto_req_aead_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_sign(const crypto_req_sign_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_verify(const crypto_req_verify_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_key_create(const crypto_req_key_create_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_key_destroy(const crypto_req_key_destroy_t *req, crypto_resp_common_t *resp);
crypto_status_t handle_encrypt_dump(const crypto_req_encrypt_dump_t *req, crypto_resp_common_t *resp);

/* Key Store Initialization */
void key_store_init(void);

/* DRBG Initialization */
void drbg_init(void);

#endif /* CRYPTO_SERVICE_H */
