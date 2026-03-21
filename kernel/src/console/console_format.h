#pragma once

#include <stddef.h>
#include <stdarg.h>

/* Minimal freestanding bounded vsnprintf.
 * Supports: %s, %c, %d, %u, %x, %p.
 * Bounded by size, strictly allocation-free.
 */
int console_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
