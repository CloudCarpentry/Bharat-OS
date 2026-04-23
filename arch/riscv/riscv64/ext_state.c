#include "../../include/arch/arch_ext_state.h"
#include "sched/sched.h"
#include "../../include/slab.h"
#include <stdint.h>
#include <stdbool.h>

#define RISCV_SSTATUS_FS_SHIFT 13
#define RISCV_SSTATUS_FS_MASK  (3UL << RISCV_SSTATUS_FS_SHIFT)
#define RISCV_SSTATUS_FS_OFF   (0UL << RISCV_SSTATUS_FS_SHIFT)
#define RISCV_SSTATUS_FS_INIT  (1UL << RISCV_SSTATUS_FS_SHIFT)
#define RISCV_SSTATUS_FS_CLEAN (2UL << RISCV_SSTATUS_FS_SHIFT)
#define RISCV_SSTATUS_FS_DIRTY (3UL << RISCV_SSTATUS_FS_SHIFT)

struct arch_ext_state {
    uint32_t flags;
    uint32_t state_flags;
    uint32_t owner_cpu;
    uint32_t reserved;
    uint64_t fpr[32];
    uint32_t fcsr;
    uint32_t pad0;
    void *rvv_blob;
    uint32_t rvv_len;
    uint32_t pad1;
};

static const arch_ext_state_desc_t g_desc = {
    .size = sizeof(arch_ext_state_t),
    .align = 16,
    .flags = ARCH_EXT_FP_SIMD,
    .eager = false,
    .lazy_allowed = true,
};

static struct bh_thread *g_fp_owner;

extern void riscv_fp_state_save(arch_ext_state_t *st);
extern void riscv_fp_state_restore(const arch_ext_state_t *st);

static inline uint64_t riscv_read_sstatus(void) {
    uint64_t v;
    __asm__ volatile("csrr %0, sstatus" : "=r"(v));
    return v;
}

static inline void riscv_write_sstatus(uint64_t v) {
    __asm__ volatile("csrw sstatus, %0" :: "r"(v) : "memory");
}

static inline void riscv_set_fs(uint64_t fs_bits) {
    uint64_t s = riscv_read_sstatus();
    s &= ~RISCV_SSTATUS_FS_MASK;
    s |= fs_bits;
    riscv_write_sstatus(s);
}

const arch_ext_state_desc_t *arch_ext_state_desc(void) { return &g_desc; }
bool arch_ext_state_enabled(void) { return true; }
void arch_ext_state_boot_init(void) { riscv_set_fs(RISCV_SSTATUS_FS_OFF); }

int arch_ext_state_thread_init(struct bh_thread *t) {
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

void arch_ext_state_thread_destroy(struct bh_thread *t) {
    if (!t || !t->cpu_context) return;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    if (g_fp_owner == t) {
        riscv_set_fs(RISCV_SSTATUS_FS_OFF);
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
        riscv_set_fs(RISCV_SSTATUS_FS_DIRTY);
        riscv_fp_state_save(st);
        st->state_flags |= ARCH_EXT_STATE_VALID;
        st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        g_fp_owner = NULL;
        riscv_set_fs(RISCV_SSTATUS_FS_OFF);
    }
}

void arch_ext_state_context_switch_in(void *next_ctx_void) {
    (void)next_ctx_void;
    riscv_set_fs(RISCV_SSTATUS_FS_OFF);
}

bool arch_ext_state_handle_fault(struct bh_thread *t) {
    if (!t || !t->cpu_context) return false;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    arch_ext_state_t *st = ctx->ext;
    if (!st) return false;

    if (g_fp_owner && g_fp_owner != t) {
        cpu_context_t *owner_ctx = (cpu_context_t *)g_fp_owner->cpu_context;
        arch_ext_state_t *owner_st = owner_ctx ? owner_ctx->ext : NULL;
        if (owner_st) {
            riscv_set_fs(RISCV_SSTATUS_FS_DIRTY);
            riscv_fp_state_save(owner_st);
            owner_st->state_flags |= ARCH_EXT_STATE_VALID;
            owner_st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        }
    }

    if (st->state_flags & ARCH_EXT_STATE_VALID) {
        riscv_set_fs(RISCV_SSTATUS_FS_DIRTY);
        riscv_fp_state_restore(st);
        riscv_set_fs(RISCV_SSTATUS_FS_CLEAN);
    } else {
        riscv_set_fs(RISCV_SSTATUS_FS_INIT);
        __asm__ volatile("csrwi fcsr, 0" ::: "memory");
        st->state_flags |= ARCH_EXT_STATE_VALID;
    }

    st->state_flags |= ARCH_EXT_STATE_LOADED;
    g_fp_owner = t;
    return true;
}

void arch_ext_state_save(struct bh_thread *t) {
    if (!t || !t->cpu_context) return;
    cpu_context_t *ctx = (cpu_context_t *)t->cpu_context;
    arch_ext_state_t *st = ctx->ext;
    if (!st) return;

    if (g_fp_owner == t) {
        riscv_set_fs(RISCV_SSTATUS_FS_DIRTY);
        riscv_fp_state_save(st);
        st->state_flags |= ARCH_EXT_STATE_VALID;
        st->state_flags &= ~ARCH_EXT_STATE_LOADED;
        g_fp_owner = NULL;
        riscv_set_fs(RISCV_SSTATUS_FS_OFF);
    }
}

void arch_ext_state_restore(struct bh_thread *t) {
    (void)t;
    // Eager restore is not used for lazy FP
}

void arch_kernel_fpu_begin(void) {
    // Stub: Handle kernel FPU begin
}

void arch_kernel_fpu_end(void) {
    // Stub: Handle kernel FPU end
}
