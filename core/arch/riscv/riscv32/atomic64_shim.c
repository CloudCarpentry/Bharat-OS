/**
 * atomic64_shim.c — Software 64-bit atomic fallbacks for RISC-V 32-bit
 *
 * On rv32, the hardware only provides 32-bit LR/SC. The compiler-rt
 * builtins __atomic_{load,store,exchange,compare_exchange}_8 are NOT
 * available in a freestanding environment without libatomic. This shim
 * implements them using a simple interrupt-disable spinlock, which is
 * correct because riscv32 in our kernel is single-core (max_supported_cores=1).
 *
 * For a future SMP riscv32 port this would need to be replaced with a real
 * spinlock or a per-line seqlock.
 */

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Inline interrupt control (no header deps)
 * --------------------------------------------------------------------- */
static inline uint32_t irq_save(void) {
    uint32_t sstatus;
    __asm__ volatile("csrrci %0, sstatus, 2" : "=r"(sstatus));
    return sstatus;
}

static inline void irq_restore(uint32_t saved) {
    if (saved & 2U) {
        __asm__ volatile("csrs sstatus, 2");
    }
}

/* -----------------------------------------------------------------------
 * __atomic_load_8
 * --------------------------------------------------------------------- */
uint64_t __atomic_load_8(const volatile void *ptr, int model) {
    (void)model;
    uint32_t s = irq_save();
    uint64_t val;
    val = *(const volatile uint64_t *)ptr;
    irq_restore(s);
    return val;
}

/* -----------------------------------------------------------------------
 * __atomic_store_8
 * --------------------------------------------------------------------- */
void __atomic_store_8(volatile void *ptr, uint64_t val, int model) {
    (void)model;
    uint32_t s = irq_save();
    *(volatile uint64_t *)ptr = val;
    irq_restore(s);
}

/* -----------------------------------------------------------------------
 * __atomic_exchange_8
 * --------------------------------------------------------------------- */
uint64_t __atomic_exchange_8(volatile void *ptr, uint64_t newval, int model) {
    (void)model;
    uint32_t s = irq_save();
    uint64_t old;
    old = *(const volatile uint64_t *)ptr;
    *(volatile uint64_t *)ptr = newval;
    irq_restore(s);
    return old;
}

/* -----------------------------------------------------------------------
 * __atomic_compare_exchange_8
 * --------------------------------------------------------------------- */
bool __atomic_compare_exchange_8(volatile void *ptr,
                                  void *expected,
                                  uint64_t desired,
                                  bool weak,
                                  int success_model,
                                  int failure_model) {
    (void)weak;
    (void)success_model;
    (void)failure_model;

    uint32_t s = irq_save();
    uint64_t cur;
    cur = *(const volatile uint64_t *)ptr;

    uint64_t exp;
    exp = *(const uint64_t *)expected;

    bool ok = (cur == exp);
    if (ok) {
        *(volatile uint64_t *)ptr = desired;
    } else {
        *(uint64_t *)expected = cur;
    }
    irq_restore(s);
    return ok;
}

/* -----------------------------------------------------------------------
 * __atomic_fetch_or_8
 * --------------------------------------------------------------------- */
uint64_t __atomic_fetch_or_8(volatile void *ptr, uint64_t val, int model) {
    (void)model;
    uint32_t s = irq_save();
    uint64_t old;
    old = *(const volatile uint64_t *)ptr;
    uint64_t newv = old | val;
    *(volatile uint64_t *)ptr = newv;
    irq_restore(s);
    return old;
}

/* -----------------------------------------------------------------------
 * __atomic_fetch_add_8
 * --------------------------------------------------------------------- */
uint64_t __atomic_fetch_add_8(volatile void *ptr, uint64_t val, int model) {
    (void)model;
    uint32_t s = irq_save();
    uint64_t old;
    old = *(const volatile uint64_t *)ptr;
    uint64_t newv = old + val;
    *(volatile uint64_t *)ptr = newv;
    irq_restore(s);
    return old;
}
