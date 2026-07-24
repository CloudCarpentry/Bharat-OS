#ifndef BHARAT_SYSCALL_CAPABILITY_H
#define BHARAT_SYSCALL_CAPABILITY_H

#include "trap/syscall_context.h"
#include <bharat/uapi/syscall/bh_syscall_status.h>

/**
 * Production-grade Syscall Capability Validation.
 *
 * These helpers perform mandatory validation of capabilities passed
 * through syscalls, including handle existence, generation matching,
 * type matching, and rights enforcement.
 */

bh_status_t bh_syscall_validate_capability(bh_syscall_ctx_t *ctx,
                                           uint32_t cap_id,
                                           uint32_t expected_type,
                                           uint64_t required_rights);

#endif /* BHARAT_SYSCALL_CAPABILITY_H */
