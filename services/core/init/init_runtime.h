#ifndef BHARAT_INIT_RUNTIME_H
#define BHARAT_INIT_RUNTIME_H

#include "init_handoff.h"

/* Execute the bootstrap runtime for the given context */
int init_runtime_start(const init_boot_context_t *ctx);

#endif /* BHARAT_INIT_RUNTIME_H */
