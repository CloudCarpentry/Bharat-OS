#ifndef BHARATLIBC_STATUS_H
#define BHARATLIBC_STATUS_H

#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t bh_status_t;

#define BHARAT_STATUS_OK                   0
#define BHARAT_STATUS_ERR_INVALID_ARG     -301
#define BHARAT_STATUS_ERR_NO_MEMORY       -302
#define BHARAT_STATUS_ERR_UNSUPPORTED     -303
#define BHARAT_STATUS_ERR_BUSY            -304
#define BHARAT_STATUS_ERR_TIMEOUT         -305
#define BHARAT_STATUS_ERR_OUT_OF_BOUNDS   -306

int bh_status_to_errno(int32_t status);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_STATUS_H */
