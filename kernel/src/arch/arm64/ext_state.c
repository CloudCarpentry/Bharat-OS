#include "../../include/arch/arch_ext_state.h"
#include "../../include/sched.h"
#include "../../include/slab.h"
#include <stdint.h>
#include <stdbool.h>

#define ARM64_CPACR_EL1_FPEN_SHIFT 20
#define ARM64_CPACR_EL1_FPEN_MASK  (3UL << ARM64_CPACR_EL1_FPEN_SHIFT)
#define ARM64_CPACR_EL1_FPEN_TRAP_ALL  (0UL << ARM64_CPACR_EL1_FPEN_SHIFT)
#define ARM64_CPACR_EL1_FPEN_ENABLE    (3UL << ARM64_CPACR_EL1_FPEN_SHIFT)

struct arch_ext_state {
    uint32_t flags;
    uint32_t state_flags;
    uint32_t owner_cpu;
    uint32_t reserved;
    __attribute__((aligned(16))) uint8_t qregs[32 * 16];
    uint32_t fpsr;
    uint32_t fpcr;
    uint32_t pad0;
    uint32_t pad1;
};

static const arch_ext_state_desc_t g_desc = {
    .size = sizeof(arch_ext_state_t),
    .align = 16,
    .flags = ARCH_EXT_FP_SIMD,
    .eager = false,
    .lazy_allowed = true,
};

static struct kthread *g_fp_owner;

extern void arm64_fp_state_save(arch_ext_state_t *st);
extern void arm64_fp_state_restore(const arch_ext_state_t *st);

static inline void arm64_fp_trap_enable(void) {
    uint64_t cpacr;
    __asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
    cpacr &= ~ARM64_CPACR_EL1_FPEN_MASK;
    cpacr |= ARM64_CPACR_EL1_FPEN_TRAP_ALL;
    __asm__ volatile("msr cpacr_el1, %0\nisb" :: "r"(cpacr) : "memory");
}

static inline void arm64_fp_access_enable(void) {
    uint64_t cpacr;
    __asm__ volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
    cpacr &= ~ARM64_CPACR_EL1_FPEN_MASK;
    cpacr |= ARM64_CPACR_EL1_FPEN_ENABLE;
    __asm__ volatile("msr cpacr_el1, %0\nisb" :: "r"(cpacr) : "memory");
}

const arch_ext_state_desc_t *arch_ext_state_desc(void) { return &g_desc; }
bool arch_ext_state_enabled(void) { return true; }
void arch_ext_state_boot_init(void) { arm64_fp_trap_enable(); }

int arch_ext_state_thread_init(struct kthread *t) {
    if (!t || !t->cpu_context) return -1;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;

    arch_ext_state_t *st = (arch_ext_state_t *)kzalloc(sizeof(*st));
    if (!st) return -1;

    st->flags = ARCH_EXT_FP_SIMD;
    st->state_flags = ARCH_EXT_STATE_TRAP_ENABLED;
    st->owner_cpu = UINT32_MAX;
    ctx->ext = st;
    return 0;
}

void arch_ext_state_thread_destroy(struct kthread *t) {
    if (!t || !t->cpu_context) return;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    if (g_fp_owner == t) {
        arm64_fp_trap_enable();
        g_fp_owner = NULL;
    }
    kfree(ctx->ext);
    ctx->ext = NULL;
}

void arch_ext_state_context_switch_out(void *prev_ctx_void) {
    cpu_context_t *ctx = (cpu_context_t *)prev_ctx_void;
    if (!ctx || !ctx->ext) return;

    if (g_fp_owner == sched_current_thread()) {
        arch_ext_state_t *st = ctx->ext;
        arm64_fp_access_enable();
        arm64_fp_state_save(st);
        st->state_flags |= ARCH_EXT_STATE_VALID;
        st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        g_fp_owner = NULL;
        arm64_fp_trap_enable();
    }
}

void arch_ext_state_context_switch_in(void *next_ctx_void) {
    cpu_context_t *ctx = (cpu_context_t *)next_ctx_void;
    (void)ctx;
    arm64_fp_trap_enable();
}

bool arch_ext_state_handle_fault(struct kthread *t) {
    if (!t || !t->cpu_context) return false;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    arch_ext_state_t *st = ctx->ext;
    if (!st) return false;

    if (g_fp_owner && g_fp_owner != t) {
        cpu_context_t *owner_ctx = (cpu_context_t *)g_fp_owner->cpu_context;
        arch_ext_state_t *owner_st = owner_ctx ? owner_ctx->ext : NULL;
        if (owner_st) {
            arm64_fp_access_enable();
            arm64_fp_state_save(owner_st);
            owner_st->state_flags |= ARCH_EXT_STATE_VALID;
            owner_st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        }
    }

    arm64_fp_access_enable();

    if (st->state_flags & ARCH_EXT_STATE_VALID) {
        arm64_fp_state_restore(st);
    } else {
        __asm__ volatile(
            "movi v0.2d, #0\n\t"  "movi v1.2d, #0\n\t"
            "movi v2.2d, #0\n\t"  "movi v3.2d, #0\n\t"
            "movi v4.2d, #0\n\t"  "movi v5.2d, #0\n\t"
            "movi v6.2d, #0\n\t"  "movi v7.2d, #0\n\t"
            "movi v8.2d, #0\n\t"  "movi v9.2d, #0\n\t"
            "movi v10.2d, #0\n\t" "movi v11.2d, #0\n\t"
            "movi v12.2d, #0\n\t" "movi v13.2d, #0\n\t"
            "movi v14.2d, #0\n\t" "movi v15.2d, #0\n\t"
            "movi v16.2d, #0\n\t" "movi v17.2d, #0\n\t"
            "movi v18.2d, #0\n\t" "movi v19.2d, #0\n\t"
            "movi v20.2d, #0\n\t" "movi v21.2d, #0\n\t"
            "movi v22.2d, #0\n\t" "movi v23.2d, #0\n\t"
            "movi v24.2d, #0\n\t" "movi v25.2d, #0\n\t"
            "movi v26.2d, #0\n\t" "movi v27.2d, #0\n\t"
            "movi v28.2d, #0\n\t" "movi v29.2d, #0\n\t"
            "movi v30.2d, #0\n\t" "movi v31.2d, #0\n\t"
            "msr fpcr, xzr\n\t"
            "msr fpsr, xzr\n\t"
            ::: "memory");
        st->state_flags |= ARCH_EXT_STATE_VALID;
    }

    st->state_flags |= ARCH_EXT_STATE_LOADED;
    g_fp_owner = t;
    return true;
}

void arch_ext_state_save(struct kthread *t) {
    if (!t || !t->cpu_context) return;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    arch_ext_state_t *st = ctx->ext;
    if (!st) return;

    if (g_fp_owner == t) {
        arm64_fp_access_enable();
        arm64_fp_state_save(st);
        st->state_flags |= ARCH_EXT_STATE_VALID;
        st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        g_fp_owner = NULL;
        arm64_fp_trap_enable();
    }
}

void arch_ext_state_restore(struct kthread *t) {
    // Eager restore is not used for lazy FP
}

void arch_kernel_fpu_begin(void) {
    // Stub: Handle kernel FPU begin
}

void arch_kernel_fpu_end(void) {
    // Stub: Handle kernel FPU end
}
