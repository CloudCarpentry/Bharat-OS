#include "../../include/arch/arch_ext_state.h"
#include "../../include/sched.h"
#include "../../include/slab.h"

static arch_ext_state_desc_t g_arch_ext_desc = {
    .size = 0,
    .align = 0,
    .flags = ARCH_EXT_NONE,
    .eager = true,
    .lazy_allowed = false
};

void arch_ext_state_boot_init(void) {
    // Stub: Calculate XSAVE area size based on capabilities
    g_arch_ext_desc.size = 512; // 512 bytes is enough for basic fxsave
    g_arch_ext_desc.align = 64; // XSAVE demands 64-byte alignment (fxsave demands 16)
    g_arch_ext_desc.flags = ARCH_EXT_NONE;
}

const arch_ext_state_desc_t *arch_ext_state_desc(void) {
    return &g_arch_ext_desc;
}

bool arch_ext_state_enabled(void) {
    return (g_arch_ext_desc.size > 0);
}

int arch_ext_state_thread_init(kthread_t *t) {
    const arch_ext_state_desc_t *d = arch_ext_state_desc();
    if (!d || d->size == 0) {
        ((cpu_context_t*)t->cpu_context)->ext = NULL;
        return 0;
    }

    // Allocate aligned extended state memory
    void *mem = kmem_aligned_alloc(d->align, d->size);
    if (!mem) {
        return -1; // -ENOMEM equivalent
    }

    // memset is safe if not empty
    unsigned char *ptr = (unsigned char *)mem;
    for(size_t i = 0; i < d->size; i++) {
        ptr[i] = 0;
    }

    ((cpu_context_t*)t->cpu_context)->ext = (arch_ext_state_t *)mem;
    return 0;
}

void arch_ext_state_thread_destroy(kthread_t *t) {
    if (t && t->cpu_context) {
        cpu_context_t *ctx = (cpu_context_t*)t->cpu_context;
        if (ctx->ext) {
            kmem_aligned_free(ctx->ext);
            ctx->ext = NULL;
        }
    }
}

void arch_ext_state_save(kthread_t *t) {
    if (!t || !((cpu_context_t*)t->cpu_context)->ext) return;
    // Stub: Eager XSAVE/FXSAVE to t->ext
}

void arch_ext_state_restore(kthread_t *t) {
    if (!t || !((cpu_context_t*)t->cpu_context)->ext) return;
    // Stub: Eager XRSTOR/FXRSTOR from t->ext
}

void arch_kernel_fpu_begin(void) {
    // Stub: Handle kernel FPU begin
}

void arch_kernel_fpu_end(void) {
    // Stub: Handle kernel FPU end
}
