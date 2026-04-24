#ifndef BHARAT_INIT_RUNTIME_H
#define BHARAT_INIT_RUNTIME_H

#include "init_profile.h"
#include "init_status.h"
#include "init_contract.h"

#define MAX_INIT_SERVICES INIT_SERVICE_ID_MAX

int init_runtime_run(init_boot_context_t *ctx);

#endif // BHARAT_INIT_RUNTIME_H
