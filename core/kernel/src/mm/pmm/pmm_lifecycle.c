#include "pmm_lifecycle.h"
#include "panic.h"
#include "atomic.h"

kstatus_t pmm_page_transition(page_t *page, pmm_page_state_t expected, pmm_page_state_t next) {
    if (!page) return K_ERR_INVALID_ARG;

    uint16_t exp = (uint16_t)expected;
    if (!__atomic_compare_exchange_n(&page->state, &exp, (uint16_t)next, false, __ATOMIC_SEQ_CST, __ATOMIC_ACQUIRE)) {
        return K_ERR_BAD_STATE;
    }

    return K_OK;
}

bool pmm_page_can_free(const page_t *page) {
    if (!page) return false;
    if (__atomic_load_n(&page->pin_count, __ATOMIC_ACQUIRE) > 0) {
        return false;
    }
    return true;
}

bool pmm_page_is_allocatable(const page_t *page) {
    if (!page) return false;
    return __atomic_load_n(&page->state, __ATOMIC_ACQUIRE) == PMM_PAGE_STATE_FREE;
}

bool pmm_page_is_pinned(const page_t *page) {
    if (!page) return false;
    return __atomic_load_n(&page->pin_count, __ATOMIC_ACQUIRE) > 0;
}

kstatus_t pmm_page_get(page_t *page) {
    if (!page) return K_ERR_INVALID_ARG;
    return bh_refcount_try_inc(&page->ref_count);
}

kstatus_t pmm_page_put(page_t *page, bool *is_last) {
    if (!page || !is_last) return K_ERR_INVALID_ARG;

    while (1) {
        uint32_t old_ref = bh_refcount_read(&page->ref_count);
        if (old_ref == 0U) {
            return K_ERR_BAD_STATE;
        }
        uint16_t pin_count = __atomic_load_n(&page->pin_count, __ATOMIC_ACQUIRE);
        if (old_ref == 1U && pin_count > 0U) {
            return K_ERR_DENIED;
        }

        if (bh_refcount_dec_and_test(&page->ref_count, is_last) == K_OK) {
            return K_OK;
        }
        // retry if raced
    }
}
