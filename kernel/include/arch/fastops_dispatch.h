#ifndef BHARAT_ARCH_FASTOPS_DISPATCH_H
#define BHARAT_ARCH_FASTOPS_DISPATCH_H

#include <stddef.h>

void fastops_dispatch_init(void);

// Function pointer typedefs
typedef void (*fast_page_zero_fn_t)(void *page);
typedef void *(*fast_memcpy_fn_t)(void *dst, const void *src, size_t n);
typedef void *(*fast_memset_fn_t)(void *s, int c, size_t n);

// Global function pointers
extern fast_page_zero_fn_t fast_page_zero;
extern fast_memcpy_fn_t fast_memcpy;
extern fast_memset_fn_t fast_memset;

#endif // BHARAT_ARCH_FASTOPS_DISPATCH_H
