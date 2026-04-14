#ifndef BHARAT_INIT_STATUS_H
#define BHARAT_INIT_STATUS_H

#include "init_manifest.h"

typedef enum {
    INIT_SERVICE_STOPPED = 0,
    INIT_SERVICE_STARTING,
    INIT_SERVICE_RUNNING,
    INIT_SERVICE_FAILED,
    INIT_SERVICE_SKIPPED,
} init_service_state_t;

typedef struct {
    const init_service_desc_t *desc;
    init_service_state_t state;
    uint8_t attempts;
    int last_error;
} init_service_runtime_t;

#define MAX_INIT_SERVICES 32

/* Get the global array of service runtimes */
init_service_runtime_t *init_status_get_runtimes(void);

/* Print a short status report */
void init_status_report(void);

#endif /* BHARAT_INIT_STATUS_H */
