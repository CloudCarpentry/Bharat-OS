#ifndef BHARAT_UAPI_IPC_H
#define BHARAT_UAPI_IPC_H

#include <stdint.h>
#include <interface/uapi/capability/capability.h>

typedef bh_cap_t bh_endpoint_t;

#define BH_IPC_SEND    (1 << 0)
#define BH_IPC_RECV    (1 << 1)
#define BH_IPC_BLOCK   (1 << 2)

struct bh_ipc_msg {
    uint64_t label;
    uint64_t args[6];
    bh_cap_t caps[2];
};

#endif /* BHARAT_UAPI_IPC_H */
