#ifndef BHARAT_INIT_STATUS_H
#define BHARAT_INIT_STATUS_H

#include "init_manifest.h"

typedef struct {
    const init_service_desc_t *desc;
    init_service_state_t state;
    uint8_t attempts;
    int last_error;
} init_service_runtime_t;

void init_status_report(const init_service_runtime_t *runtimes, size_t count);

#endif // BHARAT_INIT_STATUS_H
