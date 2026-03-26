#include "cap_policy.h"

// Returns 1 if the capability type is transferable via IPC, 0 otherwise.
int cap_can_transfer(cap_type_t type) {
    switch (type) {
    case CAP_TYPE_ENDPOINT:
    case CAP_TYPE_MEMORY:
    case CAP_TYPE_CRYPTO_ENDPOINT:
    case CAP_TYPE_CRYPTO_DEVICE:
    case CAP_TYPE_CRYPTO_KEY:
    case CAP_TYPE_RNG:
    case CAP_TYPE_SEALER:
    case CAP_TYPE_NETDEV:
    case CAP_TYPE_NET_QUEUE:
    case CAP_TYPE_NET_BUFFER:
    case CAP_TYPE_ACCEL_DEVICE:
    case CAP_TYPE_ACCEL_QUEUE:
    case CAP_TYPE_ACCEL_BUFFER:
    case CAP_TYPE_ACCEL_TELEMETRY:
    case CAP_TYPE_ACCEL_ADMIN:
    case CAP_TYPE_DMA_DOMAIN:
    case CAP_TYPE_DMA_GRANT:
        return 1;
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

// Returns 1 if the rights requested to be transferred are valid for the given capability type.
int cap_transfer_rights_valid(cap_type_t type, uint64_t transfer_rights) {
    switch (type) {
    case CAP_TYPE_ENDPOINT:
    case CAP_TYPE_CRYPTO_ENDPOINT:
        return (transfer_rights & ~(CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_MEMORY:
    case CAP_TYPE_NET_BUFFER:
        return (transfer_rights & ~(CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_MEMORY_SHARE | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_CRYPTO_DEVICE:
        return (transfer_rights & ~(CAP_RIGHT_CRYPT_ADMIN | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_CRYPTO_KEY:
        return (transfer_rights & ~(CAP_RIGHT_CRYPT_USE | CAP_RIGHT_CRYPT_DERIVE | CAP_RIGHT_CRYPT_SIGN | CAP_RIGHT_CRYPT_DECRYPT | CAP_RIGHT_CRYPT_EXPORT_WRAPPED | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_RNG:
        return (transfer_rights & ~(CAP_RIGHT_CRYPT_USE | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_SEALER:
        return (transfer_rights & ~(CAP_RIGHT_CRYPT_USE | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_NETDEV:
        return (transfer_rights & ~(CAP_RIGHT_DELEGATE)) == 0U; // Add more if needed later
    case CAP_TYPE_NET_QUEUE:
        return (transfer_rights & ~(CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_ACCEL_DEVICE:
        return (transfer_rights & ~(CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_ACCEL_QUEUE:
        return (transfer_rights & ~(CAP_RIGHT_ENQUEUE | CAP_RIGHT_CANCEL | CAP_RIGHT_QUERY | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_ACCEL_BUFFER:
        return (transfer_rights & ~(CAP_RIGHT_MEMORY_MAP | CAP_RIGHT_BIND | CAP_RIGHT_MEMORY_SHARE | CAP_RIGHT_SYNC_CPU | CAP_RIGHT_SYNC_DEV | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_ACCEL_TELEMETRY:
        return (transfer_rights & ~(CAP_RIGHT_READ_STATS | CAP_RIGHT_READ_FAULTS | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_ACCEL_ADMIN:
        return (transfer_rights & ~(CAP_RIGHT_RESET | CAP_RIGHT_PARTITION | CAP_RIGHT_FW_LOAD | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_DMA_GRANT:
        return (transfer_rights & ~(CAP_RIGHT_DMA_MAP | CAP_RIGHT_MEMORY_UNMAP | CAP_RIGHT_DELEGATE)) == 0U;
    case CAP_TYPE_DMA_DOMAIN:
        return (transfer_rights & ~(CAP_RIGHT_DELEGATE)) == 0U;
    default:
        return 0;
    }
}
