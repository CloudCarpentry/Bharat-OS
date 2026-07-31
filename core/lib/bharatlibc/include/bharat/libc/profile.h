#ifndef BHARATLIBC_PROFILE_H
#define BHARATLIBC_PROFILE_H

#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    const char *memory_model;
    const char *execution_profile;
    const char *device_profile;
    const char *personality;
} bh_libc_profile_info_t;

const bh_libc_profile_info_t *bh_libc_get_profile_info(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_PROFILE_H */
