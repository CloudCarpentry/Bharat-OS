#ifndef NETMGR_HOST_TEST_COMPAT_H
#define NETMGR_HOST_TEST_COMPAT_H

/**
 * Netmgr Host Test Compatibility Shim
 */

#ifdef BHARAT_SERVICE_HOST_TEST

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

/* Minimal Bharat type mappings */
typedef uint64_t bharat_handle_t;
typedef uint32_t bharat_cap_rights_t;

/* Status mappings */
typedef int32_t bharat_status_t;
#define BHARAT_STATUS_OK                 ((bharat_status_t)0)
#define BHARAT_STATUS_ERR_PERMISSION     ((bharat_status_t)-4)
#define BHARAT_STATUS_ERR_UNSUPPORTED    ((bharat_status_t)-6)

#define BHARAT_IPC_STATUS_OK              0
#define BHARAT_IPC_STATUS_ERR_VERSION     -2
#define BHARAT_IPC_STATUS_ERR_PERM        -4
#define BHARAT_IPC_STATUS_ERR_UNSUPPORTED -6

#endif /* BHARAT_SERVICE_HOST_TEST */

#endif // NETMGR_HOST_TEST_COMPAT_H
