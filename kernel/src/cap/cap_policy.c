#include "cap_policy.h"

// Returns 1 if the capability type is transferable via IPC, 0 otherwise.
int cap_can_transfer(cap_object_type_t type) {
    switch (type) {
    case CAP_OBJ_ENDPOINT:
    case CAP_OBJ_MEMORY:
    case CAP_OBJ_CRYPTO_ENDPOINT:
    case CAP_OBJ_CRYPTO_DEVICE:
    case CAP_OBJ_CRYPTO_KEY:
    case CAP_OBJ_RNG:
    case CAP_OBJ_SEALER:
    case CAP_OBJ_NETDEV:
    case CAP_OBJ_NET_QUEUE:
    case CAP_OBJ_NET_BUFFER:
        return 1;
    case CAP_OBJ_SCHED:
    case CAP_OBJ_PROCESS:
    case CAP_OBJ_NONE:
    case CAP_OBJ_IMPORTED_PROXY:
    default:
        // Core kernel scheduling or process capabilities cannot be transferred
        // directly as raw capabilities.
        return 0;
    }
}

// Returns 1 if the rights requested to be transferred are valid for the given capability type.
int cap_transfer_rights_valid(cap_object_type_t type, uint32_t transfer_rights) {
    switch (type) {
    case CAP_OBJ_ENDPOINT:
    case CAP_OBJ_CRYPTO_ENDPOINT:
        return (transfer_rights & ~(CAP_PERM_SEND | CAP_PERM_RECEIVE | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_MEMORY:
    case CAP_OBJ_NET_BUFFER:
        return (transfer_rights & ~(CAP_PERM_MAP | CAP_PERM_UNMAP | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_CRYPTO_DEVICE:
        return (transfer_rights & ~(CAP_PERM_CRYPT_ADMIN | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_CRYPTO_KEY:
        return (transfer_rights & ~(CAP_PERM_CRYPT_USE | CAP_PERM_CRYPT_DERIVE | CAP_PERM_CRYPT_SIGN | CAP_PERM_CRYPT_DECRYPT | CAP_PERM_CRYPT_EXPORT_WRAPPED | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_RNG:
        return (transfer_rights & ~(CAP_PERM_CRYPT_USE | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_SEALER:
        return (transfer_rights & ~(CAP_PERM_CRYPT_USE | CAP_PERM_DELEGATE)) == 0U;
    case CAP_OBJ_NETDEV:
        return (transfer_rights & ~(CAP_PERM_DELEGATE)) == 0U; // Add more if needed later
    case CAP_OBJ_NET_QUEUE:
        return (transfer_rights & ~(CAP_PERM_SEND | CAP_PERM_RECEIVE | CAP_PERM_DELEGATE)) == 0U;
    default:
        return 0;
    }
}
