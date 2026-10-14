#include <stdint.h>
#include <stddef.h>

_Static_assert(sizeof(uintptr_t) == sizeof(void *),
               "uintptr_t size mismatch");

_Static_assert(sizeof(void *) == 4 ||
               sizeof(void *) == 8,
               "unsupported pointer width");

#if defined(__aarch64__)
_Static_assert(sizeof(void *) == 8,
               "Bharat-OS ARM64 target expects 64-bit pointers");
#endif
