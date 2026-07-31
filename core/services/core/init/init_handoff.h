#ifndef BHARAT_INIT_HANDOFF_H
#define BHARAT_INIT_HANDOFF_H

#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/init/init_boot_context.h>

struct init_runtime_s; // forward declaration of init_runtime_t

int init_handoff_to_supervisor(const init_boot_context_t *ctx, struct init_runtime_s *rt);

#endif // BHARAT_INIT_HANDOFF_H
