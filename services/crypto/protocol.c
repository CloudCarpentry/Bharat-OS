#include "crypto_service.h"

crypto_status_t crypto_protocol_validate(const crypto_request_msg_t *req, uint32_t len) {
    if (len < sizeof(crypto_msg_header_t)) {
        return CRYPTO_STATUS_ERR_BAD_PARAM;
    }

    if (req->header.version != CRYPTO_UAPI_VERSION) {
        return CRYPTO_STATUS_ERR_BAD_PARAM;
    }

    if (req->header.opcode <= CRYPTO_OP_INVALID || req->header.opcode > CRYPTO_OP_ENCRYPT_DUMP) {
        return CRYPTO_STATUS_ERR_BAD_PARAM;
    }

    // In a complete implementation, capability checks would be performed here
    // e.g., verify caller has CAP_PERM_CRYPT_USE for the endpoint

    return CRYPTO_STATUS_OK;
}
