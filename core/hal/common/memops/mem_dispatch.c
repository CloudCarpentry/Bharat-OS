#include "hal/hal_memops.h"

/* Serial boot owns mutation; freeze publishes immutable per-core selections. */
static hal_memops_backend_t g_backends[HAL_MEMOPS_MAX_CPUS];
static bool g_present[HAL_MEMOPS_MAX_CPUS];
static hal_memops_current_cpu_fn_t g_current_cpu;
static bool g_frozen;

bool hal_memops_begin(hal_memops_current_cpu_fn_t current_cpu) {
    if (g_frozen || current_cpu == NULL) return false;
    for (size_t i = 0; i < HAL_MEMOPS_MAX_CPUS; ++i) g_present[i] = false;
    g_current_cpu = current_cpu;
    g_frozen = false;
    return true;
}

bool hal_memops_register(size_t cpu_id, const hal_memops_backend_t *backend) {
    if (g_frozen || backend == NULL || cpu_id >= HAL_MEMOPS_MAX_CPUS ||
        backend->copy == NULL || backend->set == NULL || backend->move == NULL) return false;
    g_backends[cpu_id] = *backend;
    g_present[cpu_id] = true;
    return true;
}

bool hal_memops_freeze(void) {
    if (g_frozen) return false;
    __atomic_store_n(&g_frozen, true, __ATOMIC_RELEASE);
    return true;
}

bool hal_memops_is_frozen(void) { return __atomic_load_n(&g_frozen, __ATOMIC_ACQUIRE); }

static const hal_memops_backend_t *selected(uint32_t flags) {
    if ((flags & (BH_MEMCTX_F_EARLY_BOOT | BH_MEMCTX_F_IRQ_SAFE)) != 0u ||
        !hal_memops_is_frozen() || g_current_cpu == NULL) return NULL;
    const size_t cpu = g_current_cpu();
    return cpu < HAL_MEMOPS_MAX_CPUS && g_present[cpu] ? &g_backends[cpu] : NULL;
}

void *hal_memcpy(void *dst, const void *src, size_t n, uint32_t flags) {
    const hal_memops_backend_t *backend = selected(flags);
    return backend == NULL ? hal_memcpy_scalar(dst, src, n) : backend->copy(dst, src, n);
}

void *hal_memset(void *dst, int c, size_t n, uint32_t flags) {
    const hal_memops_backend_t *backend = selected(flags);
    return backend == NULL ? hal_memset_scalar(dst, c, n) : backend->set(dst, c, n);
}

void *hal_memmove(void *dst, const void *src, size_t n, uint32_t flags) {
    const hal_memops_backend_t *backend = selected(flags);
    return backend == NULL ? hal_memmove_scalar(dst, src, n) : backend->move(dst, src, n);
}
