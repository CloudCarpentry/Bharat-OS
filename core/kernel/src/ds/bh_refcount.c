#include "bharat/kernel/ds/bh_refcount.h"
#include "panic.h"

void bh_refcount_init(bh_refcount_t *ref, uint32_t initial) {
    if (!ref) {
        return;
    }
    __atomic_store_n(&ref->value, initial, __ATOMIC_RELEASE);
}

uint32_t bh_refcount_read(const bh_refcount_t *ref) {
    if (!ref) {
        return 0;
    }
    return __atomic_load_n(&ref->value, __ATOMIC_ACQUIRE);
}

kstatus_t bh_refcount_try_inc(bh_refcount_t *ref) {
    if (!ref) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t old = __atomic_load_n(&ref->value, __ATOMIC_ACQUIRE);
    while (true) {
        if (old == 0) {
            return K_ERR_BAD_STATE; // Cannot resurrect from zero
        }
        if (old == UINT32_MAX) {
#ifdef BHARAT_DEBUG
            kernel_panic("bh_refcount: overflow detected");
#endif
            return K_ERR_OVERFLOW;
        }
        if (__atomic_compare_exchange_n(&ref->value, &old, old + 1, false, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)) {
            return K_OK;
        }
    }
}

kstatus_t bh_refcount_try_inc_not_zero(bh_refcount_t *ref) {
    return bh_refcount_try_inc(ref);
}

kstatus_t bh_refcount_dec_and_test(bh_refcount_t *ref, bool *is_zero) {
    if (!ref || !is_zero) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t old = __atomic_load_n(&ref->value, __ATOMIC_ACQUIRE);
    while (true) {
        if (old == 0) {
#ifdef BHARAT_DEBUG
            kernel_panic("bh_refcount: underflow detected");
#endif
            return K_ERR_BAD_STATE;
        }

        if (__atomic_compare_exchange_n(&ref->value, &old, old - 1, false, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)) {
            *is_zero = (old == 1);
            return K_OK;
        }
    }
}

bool bh_refcount_is_zero(const bh_refcount_t *ref) {
    return bh_refcount_read(ref) == 0;
}
