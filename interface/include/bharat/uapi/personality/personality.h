#ifndef BHARAT_UAPI_PERSONALITY_H
#define BHARAT_UAPI_PERSONALITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BH_PERSONALITY_NATIVE = 0,
    BH_PERSONALITY_LINUX = 1,
    BH_PERSONALITY_ANDROID = 2,
    BH_PERSONALITY_WINDOWS = 3,
    BH_PERSONALITY_POSIX_LITE = 4,
    BH_PERSONALITY_AUTOMOTIVE = 5,
    BH_PERSONALITY_ROBOTICS = 5,
    BH_PERSONALITY_MAX = 31
} bharat_personality_id_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_PERSONALITY_H
