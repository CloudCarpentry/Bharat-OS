#ifndef BHARAT_SERVICE_IDENTITY_H
#define BHARAT_SERVICE_IDENTITY_H

#include <stdint.h>

#define SERVICE_ID_INVALID 0
#define ENDPOINT_GENERATION_INITIAL 1

typedef struct service_identity {
    uint64_t service_id;
    uint32_t endpoint_gen;
} service_identity_t;

typedef struct endpoint_identity {
    uint32_t core_id;
    uint32_t endpoint_id;
    uint32_t endpoint_gen;
} endpoint_identity_t;

static inline void service_identity_bump_generation(service_identity_t* id) {
    if (id) {
        id->endpoint_gen++;
    }
}

static inline void endpoint_identity_bump_generation(endpoint_identity_t* id) {
    if (id) {
        id->endpoint_gen++;
    }
}

#endif // BHARAT_SERVICE_IDENTITY_H
