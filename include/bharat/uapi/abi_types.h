#ifndef BHARAT_UAPI_ABI_TYPES_H
#define BHARAT_UAPI_ABI_TYPES_H

#include <stdint.h>

/*
 * Stable ABI scalar IDs shared across kernel/syscalls/BIDL contracts.
 * These are intentionally fixed-width and personality-neutral.
 */
typedef uint64_t bharat_handle_t;
typedef uint64_t bharat_object_id_t;
typedef uint64_t bharat_cap_id_t;
typedef uint32_t bharat_service_id_t;
typedef uint32_t bharat_interface_id_t;
typedef uint32_t bharat_interface_version_t;

#define BHARAT_INVALID_HANDLE     ((bharat_handle_t)0ULL)
#define BHARAT_INVALID_OBJECT_ID  ((bharat_object_id_t)0ULL)
#define BHARAT_INVALID_CAP_ID     ((bharat_cap_id_t)0ULL)

#endif /* BHARAT_UAPI_ABI_TYPES_H */
