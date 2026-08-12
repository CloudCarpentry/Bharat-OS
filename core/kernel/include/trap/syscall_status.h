#ifndef BHARAT_KERNEL_SYSCALL_STATUS_H
#define BHARAT_KERNEL_SYSCALL_STATUS_H

#include "kernel/status.h"
#include <bharat/uapi/syscall/bh_syscall_status.h>

/**
 * Maps an internal kstatus_t to the native bh_status_t UAPI.
 */
bh_status_t kstatus_to_bh_status(kstatus_t st);

/**
 * Maps a kstatus_t to a long value for direct syscall return.
 * Returns bh_status_t if error, or original positive value if success.
 */
long kstatus_to_native_sysret(kstatus_t st);

/**
 * Maps a bh_status_t back to an internal kstatus_t.
 */
kstatus_t bh_status_to_kstatus(bh_status_t st);

#endif /* BHARAT_KERNEL_SYSCALL_STATUS_H */
