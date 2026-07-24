#ifndef BHARAT_DRIVERS_VERSION_H
#define BHARAT_DRIVERS_VERSION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    const char *version;
    uint32_t iface_version;
    uint32_t abi_version;
    const char *channel;
} bharat_driver_version_entry_t;

const char *bharat_driver_kernel_version(void);
size_t bharat_driver_version_count(void);
const bharat_driver_version_entry_t *bharat_driver_version_at(size_t index);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_DRIVERS_VERSION_H */
