#include "console_format.h"
#include <stdint.h>

static size_t format_uint(char **buf_ptr, size_t *size_ptr, unsigned long long value, unsigned base, int uppercase) {
    char tmp[32]; // Max length for 64-bit int in base 10/16 is < 32 chars
    int i = 0;
    size_t total_written = 0;

    if (value == 0) {
        tmp[i++] = '0';
    } else {
        while (value > 0) {
            unsigned digit = value % base;
            tmp[i++] = (digit < 10) ? ('0' + digit) : (uppercase ? ('A' + digit - 10) : ('a' + digit - 10));
            value /= base;
        }
    }

    while (i > 0) {
        char c = tmp[--i];
        if (*size_ptr > 1) {
            **buf_ptr = c;
            (*buf_ptr)++;
            (*size_ptr)--;
        }
        total_written++;
    }
    return total_written;
}

static size_t format_int(char **buf_ptr, size_t *size_ptr, long long value) {
    size_t total_written = 0;
    if (value < 0) {
        if (*size_ptr > 1) {
            **buf_ptr = '-';
            (*buf_ptr)++;
            (*size_ptr)--;
        }
        total_written++;
        value = -value;
    }
    total_written += format_uint(buf_ptr, size_ptr, (unsigned long long)value, 10, 0);
    return total_written;
}

int console_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    if (!buf && size > 0) return 0;

    char *p = buf;
    size_t remaining = size;
    size_t total_required = 0;

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (!*fmt) break;

            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(ap, int);
                    if (remaining > 1) { *p++ = c; remaining--; }
                    total_required++;
                    break;
                }
                case 's': {
                    const char *s = va_arg(ap, const char *);
                    if (!s) s = "(null)";
                    while (*s) {
                        if (remaining > 1) { *p++ = *s; remaining--; }
                        s++;
                        total_required++;
                    }
                    break;
                }
                case 'd': {
                    long long val = va_arg(ap, int);
                    total_required += format_int(&p, &remaining, val);
                    break;
                }
                case 'u': {
                    unsigned long long val = va_arg(ap, unsigned int);
                    total_required += format_uint(&p, &remaining, val, 10, 0);
                    break;
                }
                case 'x': {
                    unsigned long long val = va_arg(ap, unsigned int);
                    total_required += format_uint(&p, &remaining, val, 16, 0);
                    break;
                }
                case 'p': {
                    void *ptr = va_arg(ap, void *);
                    if (remaining > 1) { *p++ = '0'; remaining--; }
                    total_required++;
                    if (remaining > 1) { *p++ = 'x'; remaining--; }
                    total_required++;
                    total_required += format_uint(&p, &remaining, (unsigned long long)(uintptr_t)ptr, 16, 0);
                    break;
                }
                case '%': {
                    if (remaining > 1) { *p++ = '%'; remaining--; }
                    total_required++;
                    break;
                }
                default: {
                    if (remaining > 1) { *p++ = '%'; remaining--; }
                    total_required++;
                    if (remaining > 1) { *p++ = *fmt; remaining--; }
                    total_required++;
                    break;
                }
            }
        } else {
            if (remaining > 1) {
                *p++ = *fmt;
                remaining--;
            }
            total_required++;
        }
        fmt++;
    }

    if (size > 0) {
        *p = '\0';
    }

    return (int)total_required; // Total length that would have been written
}
