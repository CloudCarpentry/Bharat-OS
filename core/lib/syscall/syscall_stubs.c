#include <bharat/uapi/syscall_nr.h>
#include <bharat/uapi/sys_errno.h>

#if defined(__x86_64__)
#include <bharat/uapi/arch/x86_64/syscall.h>
#elif defined(__aarch64__)
#include <bharat/uapi/arch/arm64/syscall.h>
#elif defined(__riscv)
#include <bharat/uapi/arch/riscv64/syscall.h>
#else
#error "Unsupported architecture"
#endif

long bharat_syscall(long sysno,
                    long arg1,
                    long arg2,
                    long arg3,
                    long arg4,
                    long arg5,
                    long arg6) {
    if (sysno < 0 || (unsigned long)sysno > (unsigned long)SYSCALL_MAX) {
        return -(long)SYS_ENOSYS;
    }

    return bharat_syscall_arch(sysno, arg1, arg2, arg3, arg4, arg5, arg6);
}
