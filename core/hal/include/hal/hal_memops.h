#ifndef BHARAT_HAL_MEMOPS_H
#define BHARAT_HAL_MEMOPS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Execution context and hardware capability flags for memory operations */
#define BH_MEMCTX_F_DEFAULT        0u
#define BH_MEMCTX_F_MAY_SLEEP      (1u << 0)
#define BH_MEMCTX_F_IRQ_SAFE       (1u << 1)
#define BH_MEMCTX_F_EARLY_BOOT     (1u << 2)
#define BH_MEMCTX_F_NO_SIMD        (1u << 3)
#define BH_MEMCTX_F_NO_DMA         (1u << 4)
#define BH_MEMCTX_F_NO_FAULT       (1u << 5)

typedef void *(*hal_memcpy_backend_fn_t)(void *, const void *, size_t);
typedef void *(*hal_memset_backend_fn_t)(void *, int, size_t);
typedef void *(*hal_memmove_backend_fn_t)(void *, const void *, size_t);

typedef struct {
    hal_memcpy_backend_fn_t copy;
    hal_memset_backend_fn_t set;
    hal_memmove_backend_fn_t move;
} hal_memops_backend_t;

typedef size_t (*hal_memops_current_cpu_fn_t)(void);

#define HAL_MEMOPS_MAX_CPUS 256u

/* Serial-boot registration. Missing entries always resolve to Tier-0 scalar. */
bool hal_memops_begin(hal_memops_current_cpu_fn_t current_cpu);
bool hal_memops_register(size_t cpu_id, const hal_memops_backend_t *backend);
bool hal_memops_freeze(void);
bool hal_memops_is_frozen(void);

/* Architecture-neutral dispatched memory operations. */
void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags);
void *hal_memset(void *dst, int c, size_t n, uint32_t flags);
void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags);

/* Canonical byte-only Tier-0 fallbacks (implemented by HAL common). */
void *hal_memcpy_scalar(void *dst, const void *src, size_t n);
void *hal_memset_scalar(void *dst, int c, size_t n);
void *hal_memmove_scalar(void *dst, const void *src, size_t n);

/* Tier-0 primitives: Guaranteed non-recursive raw memory operations */
void hal_memset_raw(void *dst, int val, size_t len);

#endif /* BHARAT_HAL_MEMOPS_H */
