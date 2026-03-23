/*
 * kernel/src/arch/common/memops_scalar.c
 *
 * Provides safe, purely scalar, integer-only implementations of memcpy, memset,
 * and memmove. These are the fallback implementations used in sensitive kernel
 * contexts (like traps, IRQs, early boot) or when hardware-specific fast
 * paths are not available.
 *
 * It strictly avoids external symbols or compiler built-ins that could
 * recurse back into libc strings.
 */

#include "arch/arch_caps.h"
#include "arch/memops.h"

#define WORD_SIZE sizeof(uintptr_t)
#define PREFETCH_THRESHOLD 256

/* Fast-path state cached after initialization to avoid per-call capability probing */
static bool g_arch_is_64bit = (sizeof(uintptr_t) == 8);

/**
 * @brief Initialize architecture-specific memory operations state.
 *
 * This function resolves whether the current runtime architecture/profile
 * supports 64-bit wide virtual addresses and caches that state to avoid
 * the overhead of arch_has_cap() calls in performance-critical scalar
 * memory operations.
 */
void arch_memops_init(void) {
  /*
   * While arch_has_cap() is the preferred way to check for capability
   * presence, we cache the result here to avoid probing the capability
   * matrix on every memset call.
   */
  g_arch_is_64bit = arch_has_cap(ARCH_CAP_64BIT_VA);
}

void *arch_memcpy_scalar(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (n == 0 || dst == src) {
    return dst;
  }

  /* Optional conservative prefetch for larger copies */
  if (n >= PREFETCH_THRESHOLD) {
    __builtin_prefetch(s, 0, 0);
    __builtin_prefetch(d, 1, 0);
  }

  /* Align destination to word boundary */
  while (n > 0 && ((uintptr_t)d & (WORD_SIZE - 1)) != 0) {
    *d++ = *s++;
    n--;
  }

  /* If src is also word-aligned, we can do fast word copies */
  if (((uintptr_t)s & (WORD_SIZE - 1)) == 0) {
    uintptr_t *wd = (uintptr_t *)((void *)d);
    const uintptr_t *ws = (const uintptr_t *)((const void *)s);

    /* Unrolled loop (4 words per iteration) */
    while (n >= 4 * WORD_SIZE) {
      if (n >= PREFETCH_THRESHOLD) {
        /* Prefetch ahead by a cache line (usually 64 bytes) */
        __builtin_prefetch(ws + (64 / WORD_SIZE), 0, 0);
        __builtin_prefetch(wd + (64 / WORD_SIZE), 1, 0);
      }
      wd[0] = ws[0];
      wd[1] = ws[1];
      wd[2] = ws[2];
      wd[3] = ws[3];
      wd += 4;
      ws += 4;
      n -= 4 * WORD_SIZE;
    }

    /* Remaining words */
    while (n >= WORD_SIZE) {
      *wd++ = *ws++;
      n -= WORD_SIZE;
    }

    d = (unsigned char *)wd;
    s = (const unsigned char *)ws;
  }

  /* Scalar tail handling */
  while (n > 0) {
    *d++ = *s++;
    n--;
  }

  return dst;
}

void *arch_memset_scalar(void *dst, int c, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  unsigned char v = (unsigned char)c;

  if (n == 0) {
    return dst;
  }

  /* Optional conservative prefetch for larger copies */
  if (n >= PREFETCH_THRESHOLD) {
    __builtin_prefetch(d, 1, 0);
  }

  /* Align destination to word boundary */
  while (n > 0 && ((uintptr_t)d & (WORD_SIZE - 1)) != 0) {
    *d++ = v;
    n--;
  }

  if (n >= WORD_SIZE) {
    uintptr_t wv = v;
    wv |= wv << 8;
    wv |= wv << 16;

    if (g_arch_is_64bit) {
      /* Expand fill pattern to 64 bits only if the architecture supports it */
      wv |= (uintptr_t)((uint64_t)wv << 32);
    }

    uintptr_t *wd = (uintptr_t *)((void *)d);

    /* Unrolled loop (4 words per iteration) */
    while (n >= 4 * WORD_SIZE) {
      if (n >= PREFETCH_THRESHOLD) {
        __builtin_prefetch(wd + (64 / WORD_SIZE), 1, 0);
      }
      wd[0] = wv;
      wd[1] = wv;
      wd[2] = wv;
      wd[3] = wv;
      wd += 4;
      n -= 4 * WORD_SIZE;
    }

    /* Remaining words */
    while (n >= WORD_SIZE) {
      *wd++ = wv;
      n -= WORD_SIZE;
    }

    d = (unsigned char *)wd;
  }

  /* Scalar tail handling */
  while (n > 0) {
    *d++ = v;
    n--;
  }

  return dst;
}

void *arch_memmove_scalar(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;

  if (n == 0 || dst == src) {
    return dst;
  }

  /* Optional conservative prefetch for larger copies */
  if (n >= PREFETCH_THRESHOLD) {
    __builtin_prefetch(s, 0, 0);
    __builtin_prefetch(d, 1, 0);
  }

  if (d < s) {
    /* Forward copy */
    /* Align destination to word boundary */
    while (n > 0 && ((uintptr_t)d & (WORD_SIZE - 1)) != 0) {
      *d++ = *s++;
      n--;
    }

    if (((uintptr_t)s & (WORD_SIZE - 1)) == 0) {
      uintptr_t *wd = (uintptr_t *)((void *)d);
      const uintptr_t *ws = (const uintptr_t *)((const void *)s);

      while (n >= 4 * WORD_SIZE) {
        if (n >= PREFETCH_THRESHOLD) {
          __builtin_prefetch(ws + (64 / WORD_SIZE), 0, 0);
          __builtin_prefetch(wd + (64 / WORD_SIZE), 1, 0);
        }
        wd[0] = ws[0];
        wd[1] = ws[1];
        wd[2] = ws[2];
        wd[3] = ws[3];
        wd += 4;
        ws += 4;
        n -= 4 * WORD_SIZE;
      }

      while (n >= WORD_SIZE) {
        *wd++ = *ws++;
        n -= WORD_SIZE;
      }

      d = (unsigned char *)wd;
      s = (const unsigned char *)ws;
    }

    while (n > 0) {
      *d++ = *s++;
      n--;
    }
  } else {
    /* Backward copy */
    d += n;
    s += n;

    /* Align destination backwards to word boundary */
    while (n > 0 && ((uintptr_t)d & (WORD_SIZE - 1)) != 0) {
      *--d = *--s;
      n--;
    }

    if (((uintptr_t)s & (WORD_SIZE - 1)) == 0) {
      uintptr_t *wd = (uintptr_t *)((void *)d);
      const uintptr_t *ws = (const uintptr_t *)((const void *)s);

      while (n >= 4 * WORD_SIZE) {
        if (n >= PREFETCH_THRESHOLD) {
          __builtin_prefetch(ws - (64 / WORD_SIZE), 0, 0);
          __builtin_prefetch(wd - (64 / WORD_SIZE), 1, 0);
        }
        wd -= 4;
        ws -= 4;
        wd[3] = ws[3];
        wd[2] = ws[2];
        wd[1] = ws[1];
        wd[0] = ws[0];
        n -= 4 * WORD_SIZE;
      }

      while (n >= WORD_SIZE) {
        *--wd = *--ws;
        n -= WORD_SIZE;
      }

      d = (unsigned char *)wd;
      s = (const unsigned char *)ws;
    }

    while (n > 0) {
      *--d = *--s;
      n--;
    }
  }

  return dst;
}
