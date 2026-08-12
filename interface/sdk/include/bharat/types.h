#ifndef BHARAT_SDK_TYPES_H
#define BHARAT_SDK_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define BH_SDK_ABI_VERSION 1u
#define BH_TIMEOUT_INFINITE UINT64_MAX

typedef int32_t bh_status_t;
typedef uint32_t bh_cap_t;
typedef uint32_t bh_handle_t;
typedef uint32_t bh_endpoint_t;

enum {
    BH_OK = 0,
    BH_ERR_INVALID_ARGUMENT = -1,
    BH_ERR_UNSUPPORTED = -2,
    BH_ERR_NOT_FOUND = -3,
    BH_ERR_ACCESS_DENIED = -4,
    BH_ERR_BUSY = -5,
    BH_ERR_IO = -6
    ,BH_ERR_NO_MEMORY = -7
    ,BH_ERR_OVERFLOW = -8
    ,BH_ERR_BAD_STATE = -9
};

#endif
