#ifndef BHARAT_DRIVERS_GENERIC_DRIVER_H
#define BHARAT_DRIVERS_GENERIC_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BHARAT_GENERIC_DRIVER_CLASS_ARCH = 0,
    BHARAT_GENERIC_DRIVER_CLASS_BOARD = 1,
} bharat_generic_driver_class_t;

typedef enum {
    BHARAT_GENERIC_DRIVER_STATUS_UNINITIALIZED = 0,
    BHARAT_GENERIC_DRIVER_STATUS_READY = 1,
    BHARAT_GENERIC_DRIVER_STATUS_DEGRADED = 2,
    BHARAT_GENERIC_DRIVER_STATUS_FAILED = 3,
} bharat_generic_driver_status_t;

typedef struct {
    const char *name;
    bharat_generic_driver_class_t class_id;
    int (*init)(void);
    int (*start)(void);
    bharat_generic_driver_status_t (*health)(void);
} bharat_generic_driver_ops_t;

typedef struct {
    uint32_t initialized;
    uint32_t started;
    uint32_t failed;
} bharat_generic_driver_summary_t;

int bharat_generic_driver_bootstrap(void);
int bharat_generic_driver_start_all(void);
int bharat_generic_driver_summary(bharat_generic_driver_summary_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_DRIVERS_GENERIC_DRIVER_H */
