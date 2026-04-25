#ifndef BHARAT_UAPI_OBJECT_H
#define BHARAT_UAPI_OBJECT_H

#include <stdint.h>
#include <interface/uapi/handle/handle.h>

struct bh_object_info {
    uint32_t type;
    uint32_t rights;
    uint64_t owner_id;
};

#endif /* BHARAT_UAPI_OBJECT_H */
