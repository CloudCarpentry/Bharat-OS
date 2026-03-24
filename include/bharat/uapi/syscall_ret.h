#ifndef BHARAT_UAPI_SYSCALL_RET_H
#define BHARAT_UAPI_SYSCALL_RET_H

#include <stdint.h>
#include <bharat/uapi/sys_errno.h>

typedef int64_t bharat_sysret_t;

/*
 * Syscall return convention:
 *   >= 0 : success payload/value
 *   <  0 : -sys_errno_t
 */
#define BHARAT_SYSRET_OK(v)            ((bharat_sysret_t)(v))
#define BHARAT_SYSRET_ERR(e)           (-(bharat_sysret_t)(e))
#define BHARAT_SYSRET_IS_ERR(v)        ((bharat_sysret_t)(v) < 0)
#define BHARAT_SYSRET_TO_ERRNO(v)      ((sys_errno_t)(-((bharat_sysret_t)(v))))

#endif /* BHARAT_UAPI_SYSCALL_RET_H */
