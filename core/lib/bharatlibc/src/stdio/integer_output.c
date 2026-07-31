#include <bharat/libc/stdio_min.h>
#include <bharat/bsys/backend.h>
#include <standard/string.h>

int bh_print_str(const char *str) {
    if (!str) return -1;
    const bh_bsys_backend_ops_t *ops = bh_bsys_get_backend();
    if (!ops || !ops->write) {
        return -1;
    }
    uint32_t len = (uint32_t)strlen(str);
    uint32_t written = 0;
    int32_t rc = ops->write(1, str, len, &written); // 1 = stdout
    return (rc == 0) ? (int)written : -1;
}

int bh_print_int(int value) {
    char buf[32];
    int i = 30;
    buf[31] = '\0';
    unsigned int uval;
    int is_negative = 0;

    if (value < 0) {
        is_negative = 1;
        uval = -value;
    } else {
        uval = value;
    }

    if (uval == 0) {
        buf[--i] = '0';
    } else {
        while (uval > 0) {
            buf[--i] = '0' + (uval % 10);
            uval /= 10;
        }
    }

    if (is_negative) {
        buf[--i] = '-';
    }

    return bh_print_str(&buf[i]);
}
