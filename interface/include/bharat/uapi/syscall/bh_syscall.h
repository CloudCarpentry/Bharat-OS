#ifndef BHARAT_UAPI_SYSCALL_H
#define BHARAT_UAPI_SYSCALL_H

#include <bharat/uapi/syscall/bh_syscall_numbers.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include <bharat/uapi/syscall_args.h>

/**
 * Main UAPI syscall entry point definition for userspace.
 */
#ifndef __ASSEMBLY__
#include <stdint.h>

/**
 * Perform a Bharat-OS syscall.
 * Implementation is architecture-specific assembly in the SDK.
 */
extern int64_t bharat_syscall(
    long sysno,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6
);

#endif /* __ASSEMBLY__ */

#endif /* BHARAT_UAPI_SYSCALL_H */
