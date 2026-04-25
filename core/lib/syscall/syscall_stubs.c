#include <bharat/uapi/syscall_nr.h>

long bharat_syscall(long sysno,
                    long arg1,
                    long arg2,
                    long arg3,
                    long arg4,
                    long arg5,
                    long arg6) {
    (void)sysno;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    return -1;
}
