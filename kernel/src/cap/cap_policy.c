#include "cap_policy.h"

static const cap_rights_mask_t k_cap_valid_rights_endpoint = (CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_memory = (CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_MEMORY_SHARE | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_crypto_device = (CAP_RIGHT_CRYPT_ADMIN | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_crypto_key = (CAP_RIGHT_CRYPT_USE | CAP_RIGHT_CRYPT_DERIVE | CAP_RIGHT_CRYPT_SIGN | CAP_RIGHT_CRYPT_DECRYPT | CAP_RIGHT_CRYPT_EXPORT_WRAPPED | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_crypto_simple = (CAP_RIGHT_CRYPT_USE | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_netdev = (CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_accel_device = (CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_accel_queue = (CAP_RIGHT_ENQUEUE | CAP_RIGHT_CANCEL | CAP_RIGHT_QUERY | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_accel_buffer = (CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_BIND | CAP_RIGHT_MEMORY_SHARE | CAP_RIGHT_SYNC_CPU | CAP_RIGHT_SYNC_DEV | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_accel_telemetry = (CAP_RIGHT_READ_STATS | CAP_RIGHT_READ_FAULTS | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_accel_admin = (CAP_RIGHT_RESET | CAP_RIGHT_PARTITION | CAP_RIGHT_FW_LOAD | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_dma_grant = (CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE);
static const cap_rights_mask_t k_cap_valid_rights_dma_domain = (CAP_RIGHT_DELEGATE);

// Internal helper: Returns valid mask for capability type, or 0 if type not transferrable
static cap_rights_mask_t cap_valid_rights_for_type(cap_type_t type) {
    switch (type) {
    case CAP_TYPE_ENDPOINT:
    case CAP_TYPE_CRYPTO_ENDPOINT:
    case CAP_TYPE_NET_QUEUE:
        return k_cap_valid_rights_endpoint;
    case CAP_TYPE_MEMORY:
    case CAP_TYPE_NET_BUFFER:
        return k_cap_valid_rights_memory;
    case CAP_TYPE_CRYPTO_DEVICE:
        return k_cap_valid_rights_crypto_device;
    case CAP_TYPE_CRYPTO_KEY:
        return k_cap_valid_rights_crypto_key;
    case CAP_TYPE_RNG:
    case CAP_TYPE_SEALER:
        return k_cap_valid_rights_crypto_simple;
    case CAP_TYPE_NETDEV:
        return k_cap_valid_rights_netdev;
    case CAP_TYPE_ACCEL_DEVICE:
        return k_cap_valid_rights_accel_device;
    case CAP_TYPE_ACCEL_QUEUE:
        return k_cap_valid_rights_accel_queue;
    case CAP_TYPE_ACCEL_BUFFER:
        return k_cap_valid_rights_accel_buffer;
    case CAP_TYPE_ACCEL_TELEMETRY:
        return k_cap_valid_rights_accel_telemetry;
    case CAP_TYPE_ACCEL_ADMIN:
        return k_cap_valid_rights_accel_admin;
    case CAP_TYPE_DMA_GRANT:
        return k_cap_valid_rights_dma_grant;
    case CAP_TYPE_DMA_DOMAIN:
        return k_cap_valid_rights_dma_domain;
    case CAP_TYPE_SCHED:
    case CAP_TYPE_PROCESS:
    case CAP_TYPE_NONE:
    case CAP_TYPE_IMPORTED_PROXY:
    default:
        // Core kernel scheduling or process capabilities cannot be transferred
        // directly as raw capabilities.
        return 0;
    }
}

// Returns 1 if the rights requested to be transferred are structurally legal for the given capability type.
int cap_transfer_rights_valid(cap_type_t type, cap_rights_mask_t requested) {
    cap_rights_mask_t allowed = cap_valid_rights_for_type(type);
    if (allowed == 0) {
        return 0; // Type not transferable or unknown
    }
    // Fail closed: requested rights must be a subset of the inherently allowed rights for the type
    return (requested & ~allowed) == 0;
}

// Returns 1 if the capability type and requested rights are legal and a subset of source rights, 0 otherwise.
int cap_can_transfer(cap_type_t type, cap_rights_mask_t src_rights, cap_rights_mask_t requested) {
    if (!cap_transfer_rights_valid(type, requested)) {
        return 0;
    }
    // Requested rights must be a subset of the source capability's rights
    if ((requested & ~src_rights) != 0) {
        return 0;
    }
    return 1;
}
