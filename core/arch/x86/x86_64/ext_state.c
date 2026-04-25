#include "../../include/arch/arch_ext_state.h"
#include "sched/sched.h"
#include "../../include/slab.h"
#include "arch/arch_cpu_caps.h"

// Assembly-visible XSAVE policy knobs.
uint32_t g_x86_ext_state_use_xsave = 0;
uint32_t g_x86_ext_state_xcr0_lo = 0;
uint32_t g_x86_ext_state_xcr0_hi = 0;

static arch_ext_state_desc_t g_arch_ext_desc = {
    .size = 0,
    .align = 0,
    .flags = ARCH_EXT_NONE,
    .eager = true,
    .lazy_allowed = false
};

static inline uint64_t x86_read_xcr0(void) {
    uint32_t eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return ((uint64_t)edx << 32) | eax;
}

void arch_ext_state_boot_init(void) {
    // Baseline x87/SSE save area.
    g_arch_ext_desc.size = 512;
    g_arch_ext_desc.align = 64;
    g_arch_ext_desc.flags = ARCH_EXT_FP_SIMD;

    const bool avx_usable = arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_AVX);

    if (avx_usable) {
        const uint64_t xcr0 = x86_read_xcr0();

        // Enable XSAVE path only when kernel is actually using AVX state.
        g_x86_ext_state_use_xsave = 1;
        g_x86_ext_state_xcr0_lo = (uint32_t)(xcr0 & 0xFFFFFFFFu);
        g_x86_ext_state_xcr0_hi = (uint32_t)(xcr0 >> 32);

        // Conservative, aligned XSAVE image reservation.
        g_arch_ext_desc.size = 1024;
        g_arch_ext_desc.flags |= ARCH_EXT_VECTOR;
    }
}

const arch_ext_state_desc_t *arch_ext_state_desc(void) {
    return &g_arch_ext_desc;
}

bool arch_ext_state_enabled(void) {
    return (g_arch_ext_desc.size > 0);
}

int arch_ext_state_thread_init(bh_thread_t *t) {
    const arch_ext_state_desc_t *d = arch_ext_state_desc();
    if (!d || d->size == 0) {
        ((cpu_context_t*)t->cpu_context)->ext = NULL;
        return 0;
    }

    void *mem = kmem_aligned_alloc(d->align, d->size);
    if (!mem) {
        return -1;
    }

    unsigned char *ptr = (unsigned char *)mem;
    for (size_t i = 0; i < d->size; i++) {
        ptr[i] = 0;
    }

    ((cpu_context_t*)t->cpu_context)->ext = (arch_ext_state_t *)mem;
    return 0;
}

void arch_ext_state_thread_destroy(bh_thread_t *t) {
    if (t && t->cpu_context) {
        cpu_context_t *ctx = (cpu_context_t*)t->cpu_context;
        if (ctx->ext) {
            kmem_aligned_free(ctx->ext);
            ctx->ext = NULL;
        }
    }
}

void arch_ext_state_save(bh_thread_t *t) {
    if (!t || !((cpu_context_t*)t->cpu_context)->ext) return;
}

void arch_ext_state_restore(bh_thread_t *t) {
    if (!t || !((cpu_context_t*)t->cpu_context)->ext) return;
}

void arch_kernel_fpu_begin(void) {
}

void arch_kernel_fpu_end(void) {
}

bool arch_ext_state_handle_fault(struct bh_thread *t) {
    (void)t;
    return false;
}
