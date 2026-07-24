#ifndef BHARAT_CRYPTO_UAPI_H
#define BHARAT_CRYPTO_UAPI_H

#include <stdint.h>

/* ABI Versioning */
#define CRYPTO_UAPI_VERSION 1

/* Message Constraints */
#define CRYPTO_MAX_INLINE_DATA 256

/* Message Flags */
#define CRYPTO_MSG_F_INBUF_SHARED   (1U << 0)
#define CRYPTO_MSG_F_OUTBUF_SHARED  (1U << 1)
#define CRYPTO_MSG_F_ALLOW_HW_ACCEL (1U << 2)

/* Crypto Service Operation Opcodes */
typedef enum {
    CRYPTO_OP_INVALID          = 0,
    CRYPTO_OP_GET_RANDOM       = 1,
    CRYPTO_OP_HASH_INIT        = 2,
    CRYPTO_OP_HASH_UPDATE      = 3,
    CRYPTO_OP_HASH_FINAL       = 4,
    CRYPTO_OP_AEAD_SEAL        = 5,
    CRYPTO_OP_AEAD_OPEN        = 6,
    CRYPTO_OP_SIGN             = 7,
    CRYPTO_OP_VERIFY           = 8,
    CRYPTO_OP_KEY_CREATE       = 9,
    CRYPTO_OP_KEY_DESTROY      = 10,
    CRYPTO_OP_ENCRYPT_DUMP     = 11,
} crypto_opcode_t;

/* Crypto Service Status Codes */
typedef enum {
    CRYPTO_STATUS_OK               = 0,
    CRYPTO_STATUS_ERR_NOT_IMPL     = -1,
    CRYPTO_STATUS_ERR_BAD_PARAM    = -2,
    CRYPTO_STATUS_ERR_DENIED       = -3,
    CRYPTO_STATUS_ERR_INTERNAL     = -4,
    CRYPTO_STATUS_ERR_NO_MEMORY    = -5,
    CRYPTO_STATUS_ERR_NO_ENTROPY   = -6,
    CRYPTO_STATUS_ERR_AUTH_FAILED  = -7,
    CRYPTO_STATUS_ERR_BAD_STATE    = -8,
} crypto_status_t;

/* Common Message Header */
typedef struct {
    uint32_t version;
    crypto_opcode_t opcode;
    uint32_t flags;
    uint32_t caller_id;
    uint64_t request_id;
} crypto_msg_header_t;

/* Specific Request Structures */

typedef struct {
    uint32_t requested_bytes;
    uint64_t out_buffer_addr; /* Shared buffer address if F_OUTBUF_SHARED is set */
} crypto_req_get_random_t;

typedef struct {
    uint32_t alg_id;
    uint64_t session_id;
    uint64_t in_buffer_addr;
    uint32_t in_len;
} crypto_req_hash_t;

typedef struct {
    uint32_t alg_id;
    uint32_t key_id;
    uint64_t in_buffer_addr;
    uint32_t in_len;
    uint64_t out_buffer_addr;
    uint32_t out_capacity;
    uint64_t nonce_addr;
    uint32_t nonce_len;
    uint64_t aad_addr;
    uint32_t aad_len;
} crypto_req_aead_t;

typedef struct {
    uint32_t key_id;
    uint32_t alg_id;
    uint64_t data_addr;
    uint32_t data_len;
    uint64_t sig_buffer_addr;
    uint32_t sig_capacity;
} crypto_req_sign_t;

typedef struct {
    uint32_t key_id;
    uint32_t alg_id;
    uint64_t data_addr;
    uint32_t data_len;
    uint64_t sig_addr;
    uint32_t sig_len;
} crypto_req_verify_t;

typedef struct {
    uint32_t key_type;
    uint32_t key_size;
    uint32_t usage_flags;
} crypto_req_key_create_t;

typedef struct {
    uint32_t key_id;
} crypto_req_key_destroy_t;

typedef struct {
    uint64_t dump_addr;
    uint32_t dump_size;
    uint64_t out_addr;
    uint32_t out_capacity;
} crypto_req_encrypt_dump_t;

/* Tagged Request Union */
typedef struct {
    crypto_msg_header_t header;
    union {
        crypto_req_get_random_t    get_random;
        crypto_req_hash_t          hash;
        crypto_req_aead_t          aead;
        crypto_req_sign_t          sign;
        crypto_req_verify_t        verify;
        crypto_req_key_create_t    key_create;
        crypto_req_key_destroy_t   key_destroy;
        crypto_req_encrypt_dump_t  encrypt_dump;
        uint8_t                    inline_data[CRYPTO_MAX_INLINE_DATA];
    } payload;
} crypto_request_msg_t;

/* Common Response Structure */
typedef struct {
    crypto_msg_header_t header;
    crypto_status_t status;
    uint32_t bytes_written;
    uint32_t out_key_id;      /* Returned for key create/import operations */
    uint64_t out_session_id;  /* Returned for init operations */
    union {
        uint8_t inline_data[CRYPTO_MAX_INLINE_DATA];
    } payload;
} crypto_resp_common_t;

#endif /* BHARAT_CRYPTO_UAPI_H */
