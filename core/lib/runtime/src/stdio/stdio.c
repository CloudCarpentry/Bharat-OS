#include <string.h>
#include <stdarg.h>

#ifdef BHARAT_HOST_TEST
#include <stdio.h>
#define BH_WRITE(buf, len) fwrite(buf, 1, len, stdout)
#else
#include <stdio.h>
#include <interface/uapi/syscall/syscall_nr.h>
#include <core/lib/syscall/common/syscall.h>
#define BH_WRITE(buf, len) bh_syscall(SYSCALL_IPC_SEND, 1, (long)buf, (long)len, 0, 0, 0)
#endif

/* Simple itoa for minimal printf */
static char* itoa(long val, int base) {
    static char buf[32] = {0};
    int i = 30;
    if (val == 0) return "0";
    for (; val && i ; --i, val /= base)
        buf[i] = "0123456789abcdef"[val % base];
    return &buf[i+1];
}

#ifndef BHARAT_HOST_TEST
int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buf[256];
    char* p = buf;
    const char* f = format;

    while (*f && (p - buf < 255)) {
        if (*f == '%') {
            f++;
            if (*f == 's') {
                char* s = va_arg(args, char*);
                while (*s && (p - buf < 255)) *p++ = *s++;
            } else if (*f == 'd') {
                long d = va_arg(args, long);
                char* s = itoa(d, 10);
                while (*s && (p - buf < 255)) *p++ = *s++;
            } else if (*f == 'p' || *f == 'x') {
                long x = va_arg(args, long);
                char* s = itoa(x, 16);
                while (*s && (p - buf < 255)) *p++ = *s++;
            }
        } else {
            *p++ = *f;
        }
        f++;
    }
    *p = '\0';

    size_t len = p - buf;
    BH_WRITE(buf, len);

    va_end(args);
    return (int)len;
}
#endif
