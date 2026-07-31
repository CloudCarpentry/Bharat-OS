#include <standard/errno.h>

/* Use C11 thread-local if available */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
static _Thread_local int bh_errno_val = 0;
#else
static int bh_errno_val = 0;
#endif

int *__bh_libc_errno_location(void) {
    return &bh_errno_val;
}
