#ifndef BHARAT_UAPI_SYSCALL_STATUS_H
#define BHARAT_UAPI_SYSCALL_STATUS_H

#include <stdint.h>

/**
 * Bharat-OS Native Syscall Status Codes
 *
 * ABI rule: these codes are stable.
 * Positive values are reserved for success-with-payload.
 * Negative values are reserved for errors.
 */
typedef enum bh_status {
    BH_OK = 0,

    /* Generic Errors */
    BH_ERR_INVALID_ARGUMENT = -1,
    BH_ERR_INVALID_SYSCALL  = -2,
    BH_ERR_NOT_SUPPORTED    = -3,
    BH_ERR_ACCESS_DENIED    = -4,
    BH_ERR_FAULT            = -5,
    BH_ERR_NO_MEMORY        = -6,
    BH_ERR_TRY_AGAIN        = -7,
    BH_ERR_INTERRUPTED      = -8,
    BH_ERR_BAD_STATE        = -9,
    BH_ERR_TIMEOUT          = -10,
    BH_ERR_OVERFLOW         = -11,
    BH_ERR_INTERNAL         = -12,

    /* Capability & Security Errors */
    BH_ERR_BAD_CAPABILITY   = -20,
    BH_ERR_STALE_CAPABILITY = -21,
    BH_ERR_WRONG_TYPE       = -22,
    BH_ERR_INSUFFICIENT_RIGHTS = -23,

    /* IPC & Resource Errors */
    BH_ERR_CHANNEL_CLOSED   = -30,
    BH_ERR_BUFFER_FULL      = -31,
    BH_ERR_RESOURCE_EXHAUSTED = -32,
    BH_ERR_NOT_FOUND        = -33,

} bh_status_t;

#endif /* BHARAT_UAPI_SYSCALL_STATUS_H */
