#ifndef BHARATLIBC_BUILD_INFO_H
#define BHARATLIBC_BUILD_INFO_H

#include <standard/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *version;
    const char *build_date;
    const char *build_type;
    const char *target_arch;
} bh_libc_build_info_t;

const bh_libc_build_info_t *bh_libc_get_build_info(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_BUILD_INFO_H */
