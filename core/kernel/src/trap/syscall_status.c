#include "trap/syscall_status.h"

bh_status_t kstatus_to_bh_status(kstatus_t st) {
    if (st >= 0) return BH_OK;

    switch (st) {
        case K_ERR_INVALID_ARG:       return BH_ERR_INVALID_ARGUMENT;
        case K_ERR_INVALID_SYSCALL:   return BH_ERR_INVALID_SYSCALL;
        case K_ERR_BAD_STATE:         return BH_ERR_BAD_STATE;
        case K_ERR_NOT_FOUND:         return BH_ERR_NOT_FOUND;
        case K_ERR_UNSUPPORTED:       return BH_ERR_NOT_SUPPORTED;
        case K_ERR_TIMEOUT:           return BH_ERR_TIMEOUT;
        case K_ERR_AGAIN:
        case K_ERR_RETRY:             return BH_ERR_TRY_AGAIN;
        case K_ERR_INTERRUPTED:       return BH_ERR_INTERRUPTED;
        case K_ERR_FAULT:             return BH_ERR_FAULT;
        case K_ERR_DENIED:            return BH_ERR_ACCESS_DENIED;
        case K_ERR_OVERFLOW:          return BH_ERR_OVERFLOW;
        case K_ERR_NO_MEMORY:         return BH_ERR_NO_MEMORY;

        /* Capability */
        case K_ERR_CAP_INVALID:       return BH_ERR_BAD_CAPABILITY;
        case K_ERR_CAP_WRONG_TYPE:    return BH_ERR_WRONG_TYPE;
        case K_ERR_CAP_DENIED:        return BH_ERR_INSUFFICIENT_RIGHTS;
        case K_ERR_CAP_STALE:         return BH_ERR_STALE_CAPABILITY;

        /* IPC */
        case K_ERR_IPC_CLOSED:        return BH_ERR_CHANNEL_CLOSED;
        case K_ERR_IPC_QUEUE_FULL:    return BH_ERR_BUFFER_FULL;

        /* Resource */
        case K_ERR_PMM_EXHAUSTED:     return BH_ERR_RESOURCE_EXHAUSTED;

        default:                      return BH_ERR_INTERNAL;
    }
}

long kstatus_to_native_sysret(kstatus_t st) {
    if (st >= 0) return (long)st;
    return (long)kstatus_to_bh_status(st);
}
