#ifndef BHARAT_VERSION_API_H
#define BHARAT_VERSION_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bharat_component_version;

/* Retrieves the component info struct embedded in this binary's .bharat.version section */
const struct bharat_component_version *bharat_get_component_info(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_VERSION_API_H */
