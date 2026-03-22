#ifndef BHARAT_NETWORK_ERRORS_H
#define BHARAT_NETWORK_ERRORS_H

/* Stable networking error codes layered over system errno-style values */

#define BNET_OK             0

#define BNET_ERR_INVAL     -1   /* Invalid argument */
#define BNET_ERR_NOENT     -2   /* No such file or directory / not found */
#define BNET_ERR_NOMEM     -3   /* Out of memory */
#define BNET_ERR_ACCESS    -4   /* Permission denied / Capability fault */
#define BNET_ERR_BUSY      -5   /* Device or resource busy */
#define BNET_ERR_EXIST     -6   /* File or resource exists */
#define BNET_ERR_NOTSUP    -7   /* Operation not supported */
#define BNET_ERR_IO        -8   /* I/O error */
#define BNET_ERR_TIMEOUT   -9   /* Operation timed out */

/* Network specific errors */
#define BNET_ERR_NETDOWN   -100 /* Network is down */
#define BNET_ERR_NETUNREACH -101 /* Network is unreachable */
#define BNET_ERR_HOSTUNREACH -102 /* No route to host */
#define BNET_ERR_CONNREFUSED -103 /* Connection refused */
#define BNET_ERR_ADDRINUSE   -104 /* Address already in use */
#define BNET_ERR_ADDRNOTAVAIL -105 /* Cannot assign requested address */

#endif // BHARAT_NETWORK_ERRORS_H
