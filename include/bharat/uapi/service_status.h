#ifndef BHARAT_UAPI_SERVICE_STATUS_H
#define BHARAT_UAPI_SERVICE_STATUS_H

#include <stdint.h>

/*
 * Service-contract status space (BIDL/L2-L3 contracts).
 * Distinct from syscall errno transport semantics.
 */
typedef int32_t bharat_status_t;

#define BHARAT_STATUS_OK                 ((bharat_status_t)0)
#define BHARAT_STATUS_ERR_DECODE         ((bharat_status_t)-1)
#define BHARAT_STATUS_ERR_VERSION        ((bharat_status_t)-2)
#define BHARAT_STATUS_ERR_OPCODE         ((bharat_status_t)-3)
#define BHARAT_STATUS_ERR_PERMISSION     ((bharat_status_t)-4)
#define BHARAT_STATUS_ERR_NOT_FOUND      ((bharat_status_t)-5)
#define BHARAT_STATUS_ERR_UNSUPPORTED    ((bharat_status_t)-6)
#define BHARAT_STATUS_ERR_INTERNAL       ((bharat_status_t)-7)
#define BHARAT_STATUS_ERR_TRUNCATED      ((bharat_status_t)-8)
#define BHARAT_STATUS_ERR_LENGTH         ((bharat_status_t)-9)
#define BHARAT_STATUS_ERR_FLAGS          ((bharat_status_t)-10)

#endif /* BHARAT_UAPI_SERVICE_STATUS_H */
