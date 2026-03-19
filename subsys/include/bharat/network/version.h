#ifndef BHARAT_NETWORK_VERSION_H
#define BHARAT_NETWORK_VERSION_H

/* ABI version macros and compatibility guards */

#define BNET_ABI_VERSION_MAJOR 1
#define BNET_ABI_VERSION_MINOR 0

#define BNET_ABI_VERSION ((BNET_ABI_VERSION_MAJOR << 16) | (BNET_ABI_VERSION_MINOR))

#define BNET_CHECK_ABI_COMPAT(client_version) \
    (((client_version) >> 16) == BNET_ABI_VERSION_MAJOR)

#endif // BHARAT_NETWORK_VERSION_H
